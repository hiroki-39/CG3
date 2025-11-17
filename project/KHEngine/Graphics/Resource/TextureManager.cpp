#include "KHEngine/Graphics/Resource/TextureManager.h"
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

void TextureManager::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	// SRVの数と同数
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath)
{

	// すでに読み込まれているかチェック
	for (auto& t : textureDatas)
	{
		if (t.filePath == filePath)
		{
			return;
		}
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
	textureDatas.resize(textureDatas.size() + 1);

	// 追加したテクスチャデータの参照を取得
	TextureData& textureData = textureDatas.back();

	//ファイルから読み取った情報を格納
	textureData.filePath = filePath;
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxCommon_->CreateTextureResource(textureData.metadata);

	// テクスチャデータの要素数番号をSRVのインデックスにする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;

	textureData.srvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);

	// SRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// 設定をもとにSRVの生成
	dxCommon_->GetDevice()->CreateShaderResourceView(
		textureData.resource.Get(),
		&srvDesc,
		textureData.srvHandleCPU
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
	for (auto& tex : textureDatas)
	{
		tex.subresources.clear();
		tex.intermediateResource.Reset();
	}
}

void TextureManager::ExecuteUploadCommands()
{
	dxCommon_->ExecuteTextureUploadBatch();
}


uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	auto it = std::find_if(
		textureDatas.begin(),
		textureDatas.end(),
		[&](const TextureData& data)
		{
			return data.filePath == filePath;
		}
	);

	if (it != textureDatas.end())
	{
		// 見つかったらSRVインデックスを返す
		size_t index = std::distance(textureDatas.begin(), it);
		return static_cast<uint32_t>(index) + kSRVIndexTop;
	}

	assert(0);

	// 見つからなかったら0を返す
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex)
{
	// 範囲外指定違反チェック
	assert(textureIndex >= kSRVIndexTop && textureIndex < textureDatas.size() + kSRVIndexTop);

	// テクスチャデータの参照を取得
	TextureData& textureData = textureDatas[textureIndex - kSRVIndexTop];

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
