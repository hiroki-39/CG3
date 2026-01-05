#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"

class DirectXCommon; 

class SrvManager
{
public:// メンバ関数

	static SrvManager* GetInstance();

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	void Finalize();

	/// <summary>
	///　確保用
	/// </summary>
	uint32_t Allocate();

	/// <summary>
	/// SRVの指定番号のCPUデスクリプタハンドルを取得
	/// </summary>
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// SRVの指定番号のGPUデスクリプタハンドルを取得
	/// </summary>
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);

	/// <summary>
	/// SRV作成(Texture2D用)
	/// </summary>
	void CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT Format, UINT MipLevels);

	/// <summary>
	/// SRV作成(StructuredBuffer用)
	/// </summary>
	void CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT NumElements, UINT StructureByteStride);

	/// <summary>
	/// 
	/// </summary>
	void PreDraw();

	/// <summary>
	///
	/// </summary>
	void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex);

	/// <summary>
	/// 確保可能かチェック
	/// </summary>
	bool CanAllocate();

public:

	// 最大SRV数　(最大テクスチャ枚数)
	static const uint32_t kMaxSRVCount;

private:

	DirectXCommon* directXCommon = nullptr;

	// SRV用のDescriptorSize
	uint32_t descriptorSize;

	// SRVのヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;

	// 次に使用するSRVインデックス
	uint32_t useIndex = 0;
};

