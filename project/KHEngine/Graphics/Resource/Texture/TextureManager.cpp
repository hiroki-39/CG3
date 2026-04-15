#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Core/Resource/ResourceLocator.h"
#include "KHEngine/Core/Utility/String/StringUtility.h"
#include <cassert>

// UI表示用に 1 を足すための定数（内部は生インデックスを使う）
uint32_t TextureManager::kSRVIndexTop = 1;


TextureManager* TextureManager::GetInstance()
{
	static TextureManager instance;
	return &instance;
}

void TextureManager::Finalize()
{
	// 内部リソースの解放（ComPtr とコンテナの自動管理に任せる）
	textureDatas.clear();
	textureIndexToFilePath.clear();
	dxCommon_ = nullptr;
	srvManager = nullptr;
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
	// 論理名またはパスを実パスに解決
	std::string resolved = ResourceLocator::Resolve(filePath, ResourceLocator::AssetType::Texture);

	// 存在しない場合はそのまま試す（古いコード互換）
	if (resolved.empty())
	{
		resolved = filePath;
	}

	// すでに読み込まれているかチェック（キーは実パス）
	if (textureDatas.find(resolved) != textureDatas.end())
	{
		return;
	}

	// テクスチャ枚数上限チェック
	assert(textureDatas.size() + 1 < SrvManager::kMaxSRVCount);

	// ファイルからテクスチャデータを読み込む
	DirectX::ScratchImage image{};
	HRESULT hr = DirectX::LoadFromWICFile(
		StringUtility::ConvertString(resolved).c_str(),
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


	// テクスチャデータを追加（key=resolved）
	textureDatas.emplace(resolved, TextureData{});

	// 追加したテクスチャデータの参照を取得
	TextureData& textureData = textureDatas[resolved];

	//ファイルから読み取った情報を格納
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

	// テクスチャデータの要素数番号をSRVのインデックスにする（生インデックス）
	textureData.srvIndex = srvManager->Allocate();

	// インデックス -> ファイルパス マッピングを追加（srvIndex と配列インデックスを一致させる）
	if (textureIndexToFilePath.size() <= textureData.srvIndex) {
		textureIndexToFilePath.resize(textureData.srvIndex + 1);
	}
	textureIndexToFilePath[textureData.srvIndex] = resolved;

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

	// image/mipImages の所有権を textureData.image に移動（明示的 Release は行わない）
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
		textureData.image = DirectX::ScratchImage();
	}
}

void TextureManager::ExecuteUploadCommands()
{
	dxCommon_->ExecuteTextureUploadBatch();
}


uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
	// 解決して実パスキーで検索
	std::string resolved = ResourceLocator::Resolve(filePath, ResourceLocator::AssetType::Texture);
	if (resolved.empty()) resolved = filePath;

	// 読み込み済みテクスチャを検索
	if (textureDatas.contains(resolved))
	{
		TextureData& textureData = textureDatas[resolved];
		return textureData.srvIndex;
	}

	// テクスチャ条件をチェック
	assert(srvManager->CanAllocate());

	// 見つからなかったら0を返す
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath)
{
	std::string resolved = ResourceLocator::Resolve(filePath, ResourceLocator::AssetType::Texture);
	if (resolved.empty()) resolved = filePath;

	assert(textureDatas.contains(resolved));
	TextureData& textureData = textureDatas[resolved];

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
	std::string resolved = ResourceLocator::Resolve(filePath, ResourceLocator::AssetType::Texture);
	if (resolved.empty()) resolved = filePath;

	if (textureDatas.contains(resolved))
	{
		TextureData& textureData = textureDatas[resolved];
		return textureData.srvIndex;
	}

	assert(srvManager->CanAllocate());

	return 0;
}