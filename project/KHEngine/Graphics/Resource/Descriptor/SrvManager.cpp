#include "SrvManager.h"

const uint32_t SrvManager::kMaxSRVCount = 512;

SrvManager* SrvManager::GetInstance()
{
	static SrvManager* instance = new SrvManager();
	return instance;
}

void SrvManager::Finalize()
{
	delete GetInstance();
}

void SrvManager::Initialize(DirectXCommon* dxCommon)
{
	directXCommon = dxCommon;

	// デスクリプタヒープの生成
	descriptorHeap = directXCommon->CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxSRVCount, true);

	// デスクリプタ1個分のサイズを取得取得して記録
	descriptorSize = directXCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

uint32_t SrvManager::Allocate()
{
	// 上限チェックしてassertをかける
	assert(useIndex < kMaxSRVCount);

	// 使用する番号を退避
	int index = useIndex;

	// 次回のために番号を1進める
	useIndex++;

	return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetSRVCPUDescriptorHandle(uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetSRVGPUDescriptorHandle(uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

void SrvManager::CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels)
{
	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(MipLevels);
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// 設定をもとにSRVの生成
	directXCommon->GetDevice()->CreateShaderResourceView(
		pResource,
		&srvDesc,
		GetSRVCPUDescriptorHandle(srvIndex)
	);
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT NumElements, UINT StructureByteStride)
{
	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = NumElements;
	srvDesc.Buffer.StructureByteStride = StructureByteStride;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// 設定をもとにSRVの生成
	directXCommon->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetSRVCPUDescriptorHandle(srvIndex));
}

void SrvManager::PreDraw()
{
	// デスクリプタヒープの設定
	ID3D12DescriptorHeap* ppHeaps[] = { descriptorHeap.Get() };
	directXCommon->GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex)
{
	directXCommon->GetCommandList()->SetGraphicsRootDescriptorTable(rootParameterIndex,GetSRVGPUDescriptorHandle(srvIndex));
}

bool SrvManager::CanAllocate()
{
	//上限に足していればtrueを返す、そうでなければfalseを返す
	return useIndex < kMaxSRVCount;
}
