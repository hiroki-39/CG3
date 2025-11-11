#include "KHEngine/Graphics/Resource/TextureManager.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"

TextureManager* TextureManager::instance = nullptr;

// ImGuiで0番目を使うためSRVインデックスを1から始める
uint32_t TextureManager::kSRVIndexTop = 1;


TextureManager* TextureManager::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new TextureManager;
	}

	return instance;
}

void TextureManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void TextureManager::Initialize()
{
	// SRVの数と同数
	textureDatas.reserve(DirectXCommon::kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath)
{

	DirectX::ScratchImage image{};

	//テクスチャファイルを読んでプログラムで扱えるようにする

	DirectX::ScratchImage mipImages{};

	//ミップマップの作成
	HRESULT hr = DirectX::GenerateMipMaps(
		image.GetImages(),
		image.GetImageCount(),
		image.GetMetadata(),
		DirectX::TEX_FILTER_SRGB,
		0,
		mipImages);

	//テクスチャデータを追加
	textureDatas.resize(textureDatas.size() + 1);

	// 追加したテクスチャデータの参照を取得
	TextureData& textureData = textureDatas.back();

	//ファイルから読み取った情報を格納
	textureData.filePath = filePath;
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource =dxCommon_->CreateTextureResource(textureData.metadata);

	// テクスチャデータの要素数番号をSRVのインデックスにする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;

	textureData.srvHandleCPU = dxCommon_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = dxCommon_->GetSRVGPUDescriptorHandle(srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	// SRVの設定
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

	/*std::wstring filePathW = StringUtility::ConvertString(filePath);

	HRESULT hr = DirectX::LoadFromWICFile(
		filePathW.c_str(),
		DirectX::WIC_FLAGS_DEFAULT_SRGB,
		nullptr,
		image
	);*/
	
	// 読み込み失敗なら白テクスチャを返す
	/*if (FAILED(hr))
	{
		// 白色1x1のテクスチャを作成
		DirectX::TexMetadata metadata{};
		metadata.width = 1;
		metadata.height = 1;
		metadata.arraySize = 1;
		metadata.mipLevels = 1;
		metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

		DirectX::Image whiteImage{};
		whiteImage.width = 1;
		whiteImage.height = 1;
		whiteImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		whiteImage.rowPitch = 4;
		whiteImage.slicePitch = 4;

		uint8_t* pixels = new uint8_t[4]{ 255, 255, 255, 255 }; // 白 RGBA
		whiteImage.pixels = pixels;

		image.InitializeFromImage(whiteImage);

		// mipなしでそのまま返す
		mipImages.InitializeFromImage(whiteImage);

		delete[] pixels;
		return mipImages;
	}*/

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
		// 読み込み済みなら早期return
		return;
	}

	// テクスチャ枚数上限チェック
	assert(textureDatas.size() + kSRVIndexTop < DirectXCommon::kMaxSRVCount);

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
