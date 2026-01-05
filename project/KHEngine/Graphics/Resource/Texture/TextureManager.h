#pragma once
#include <string>
#include "externals/DirectXTex/DirectXTex.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

// テクスチャマネージャー
class TextureManager
{

public://メンバ関数

	// インスタンス取得
	static TextureManager* GetInstance();

	/// <summary>
	/// 終了処理
	/// </summary>
	void Finalize();

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Initialize(DirectXCommon* dxCommon, SrvManager* srvManager);

	/// <summary>
	/// テクスチャの読み込み
	/// </summary>
	void LoadTexture(const std::string& filePath);

	/// <summary>
	/// 中間リソースの解放
	/// </summary>
	void ClearIntermediateResources();

	/// <summary>
	/// アップロードコマンドの実行 
	/// </summary>
	void ExecuteUploadCommands();

	/// <summary>
	/// メタデータを取得
	/// </summary>
	/// <param name="textureIndex">テクスチャインデックス</param>
	const DirectX::TexMetadata& GetMetaData(uint32_t TextureIndex);

	// SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	// テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	uint32_t GetSrvIndex(const std::string& filePath);

private://静的メンバ関数

	// コンストラクタ
	TextureManager() = default;

	// デストラクタ
	~TextureManager() = default;

	TextureManager(TextureManager&) = delete;

	TextureManager& operator=(TextureManager&) = delete;

	/// <summary>
	/// サブリソースデータ作成
	/// </summary>
	/// <param name="mipImages">ミップマップ画像群</param>
	static std::vector<D3D12_SUBRESOURCE_DATA> CreateSubresources(const DirectX::ScratchImage& mipImages);

private:

	//　テクスチャ1枚分のデータ
	struct TextureData
	{
		DirectX::TexMetadata metadata;								 // テクスチャメタデータ
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;			 // テクスチャリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource; // 転送用中間リソース
		uint32_t srvIndex;											 // SRVインデックス
		std::vector<D3D12_SUBRESOURCE_DATA> subresources;			 // サブリソースデータ群
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;					 // SRVハンドル(CPU)
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;					 // SRVハンドル(GPU)
		DirectX::ScratchImage image;								 // アップロード完了までピクセルを保持
	};


	// シングルトンインスタンス
	static TextureManager* instance;

	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

	// DirectXCommon
	DirectXCommon* dxCommon_ = nullptr;

	// テクスチャデータ群
	std::unordered_map<std::string, TextureData> textureDatas;

	SrvManager* srvManager = nullptr;
};

