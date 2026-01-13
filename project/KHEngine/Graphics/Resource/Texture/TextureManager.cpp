#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"

TextureManager* TextureManager::instance = nullptr;

// UI表示用に 1 を足すための定数（内部は生インデックスを使う）
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

	// 事前確保（ヒープの最大数に合わせて）
	textureDatas.reserve(srvManager->kMaxSRVCount);
	textureIndexToFilePath.reserve(srvManager->kMaxSRVCount);
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
	assert(textureDatas.size() + 1 < DirectXCommon::kMaxSRVCount);

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

	// テクスチャデータの要素数番号をSRVのインデックスにする（生インデックス）
	textureData.srvIndex = srvManager->Allocate();

	// インデックス -> ファイルパス マッピングを追加（srvIndex と配列インデックスを一致させる）
	if (textureIndexToFilePath.size() <= textureData.srvIndex) {
		textureIndexToFilePath.resize(textureData.srvIndex + 1);
	}
	textureIndexToFilePath[textureData.srvIndex] = filePath;

	textureData.srvHandleCPU = srvManager->GetSRVCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager->GetSRVGPUDescriptorHandle(textureData.srvIndex);

	// SRVの作成
	srvManager->CreateSRVforTexture2D(
		textureData.srvIndex,
		textureData.resource.Get(),
		textureData.metadata.format,
		static_cast<UINT>(textureData.metadata.mipLevels)
	);

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
		// 見つかったらSRVインデックス（生インデックス）を返す
		TextureData& textureData = textureDatas[filePath];
		return textureData.srvIndex;
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
	// 生インデックス（0始まり）でチェック
	assert(TextureIndex < textureIndexToFilePath.size());

	// インデックス -> ファイルパス経由で参照
	const std::string& filePath = textureIndexToFilePath[TextureIndex];
	auto it = textureDatas.find(filePath);
	assert(it != textureDatas.end());

	// メタデータを返す
	return it->second.metadata;
}

uint32_t TextureManager::GetSrvIndex(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	if (textureDatas.contains(filePath))
	{
		// 見つかったらSRVインデックスを返す（生インデックス）
		TextureData& textureData = textureDatas[filePath];
		return textureData.srvIndex;
	}

	assert(srvManager->CanAllocate());

	// 見つからなかったら0を返す
	return 0;
}
