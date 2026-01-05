#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"

TextureManager* TextureManager::instance = nullptr;

// ImGuiで0番目を使うためSRVインデックスを1から始める
uint32_t TextureManager::kSRVIndexTop = 1;


TextureManager* TextureManager::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new TextureManager();
	}

	return instance;
}

void TextureManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void TextureManager::Initialize(DirectXCommon* dxCommon, SrvManager* srvManager)
{
	dxCommon_ = dxCommon;

	// メンバ変数として記録する
	this->srvManager = srvManager;

	// SRVの数と同数
	textureDatas.reserve(srvManager->kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath)
{

	// すでに読み込まれているかチェック
	if (textureDatas.find(filePath) != textureDatas.end())
	{
		// 読み込み済みなら何もしない
		return;
	}

	// テクスチャ枚数上限チェック
	assert(textureDatas.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

	// ファイルからテクスチャデータを読み込む
	DirectX::ScratchImage image{};
	HRESULT hr = DirectX::LoadFromWICFile(
		StringUtility::ConvertString(filePath).c_str(),
		DirectX::WIC_FLAGS_FORCE_SRGB,
		nullptr,
		image
	);

	assert(SUCCEEDED(hr));


	//ミップマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(
		image.GetImages(),
		image.GetImageCount(),
		image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB,
		0,
		mipImages);

	assert(SUCCEEDED(hr));


	// テクスチャデータを追加
	textureDatas.emplace(filePath, TextureData{});

	// 追加したテクスチャデータの参照を取得
	TextureData& textureData = textureDatas[filePath];

	//ファイルから読み取った情報を格納
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

	// テクスチャデータの要素数番号をSRVのインデックスにする
	textureData.srvIndex = srvManager->Allocate();
	textureData.srvHandleCPU = srvManager->GetSRVCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager->GetSRVGPUDescriptorHandle(textureData.srvIndex);

	// SRVの作成
	srvManager->CreateSRVforTexture2D(
		textureData.srvIndex,
		textureData.resource.Get(),
		textureData.metadata.format,
		static_cast<UINT>(textureData.metadata.mipLevels)
	);

	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	//srvDesc.Format = textureData.metadata.format;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.Texture2D.MostDetailedMip = 0;
	//srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);
	//srvDesc.Texture2D.PlaneSlice = 0;
	//srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	//// 設定をもとにSRVの生成
	//dxCommon_->GetDevice()->CreateShaderResourceView(
	//	textureData.resource.Get(),
	//	&srvDesc,
	//	textureData.srvHandleCPU
	//);

	// ローカルで作成したサブリソース情報（ここをtextureDataへ保存する）
	std::vector<D3D12_SUBRESOURCE_DATA> subresources = CreateSubresources(mipImages);

	// intermediateサイズは正しいサブリソース数で計算する
	uint64_t intermediateSize = GetRequiredIntermediateSize(
		textureData.resource.Get(),
		0,
		(UINT)subresources.size()
	);

	// 転送用に生成した中間リソースをテクスチャデータ構造体に格納
	textureData.intermediateResource = dxCommon_->CreateBufferResource(intermediateSize);

	// サブリソース情報を保存しておく（後でバッチ転送で使う）
	textureData.subresources = std::move(subresources);

	textureData.image = std::move(mipImages);

	dxCommon_->AddTextureUpload(textureData.resource, textureData.intermediateResource, textureData.subresources);
}

void TextureManager::ClearIntermediateResources()
{
	for (auto& [filePath, textureData] : textureDatas)
	{
		// 中間リソースを解放
		textureData.intermediateResource.Reset();
		// ピクセルデータを解放
		textureData.image.Release();
	}
}

void TextureManager::ExecuteUploadCommands()
{
	dxCommon_->ExecuteTextureUploadBatch();
}


uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	if (textureDatas.contains(filePath))
	{
		// 見つかったらSRVインデックスを返す
		TextureData& textureData = textureDatas[filePath];
		return textureData.srvIndex + kSRVIndexTop;
	}

	// テクスチャ条件をチェック
	assert(srvManager->CanAllocate());

	// 見つからなかったら0を返す
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	assert(textureDatas.contains(filePath));
	// 見つかったらSRVハンドルを返す
	TextureData& textureData = textureDatas[filePath];

	return textureData.srvHandleGPU;
}

std::vector<D3D12_SUBRESOURCE_DATA> TextureManager::CreateSubresources(const DirectX::ScratchImage& mipImages)
{
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	const DirectX::Image* images = mipImages.GetImages();

	subresources.resize(metadata.mipLevels);

	for (size_t mip = 0; mip < metadata.mipLevels; ++mip)
	{
		const DirectX::Image& img = images[mip];

		D3D12_SUBRESOURCE_DATA& sub = subresources[mip];
		sub.pData = img.pixels;
		sub.RowPitch = img.rowPitch;
		sub.SlicePitch = img.slicePitch;
	}

	return subresources;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(uint32_t TextureIndex)
{
	// 範囲外指定違反チェック
	assert(textureIndex >= kSRVIndexTop && textureIndex < textureDatas.size() + kSRVIndexTop);

	// テクスチャデータの参照を取得
	auto it = textureDatas.begin();
	std::advance(it, TextureIndex - kSRVIndexTop);
	TextureData& textureData = it->second;
	
	// メタデータを返す
	return textureData.metadata;
}

uint32_t TextureManager::GetSrvIndex(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	if (textureDatas.contains(filePath))
	{
		// 見つかったらSRVインデックスを返す
		TextureData& textureData = textureDatas[filePath];
		return textureData.srvIndex;
	}

	assert(srvManager->CanAllocate());

	// 見つからなかったら0を返す
	return 0;
}
