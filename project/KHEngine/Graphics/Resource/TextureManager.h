#pragma once
#include <string>
#include "externals/DirectXTex/DirectXTex.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include <wrl.h>
#include <d3d12.h>

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
	void Initialize();

	/// <summary>
	/// テクスチャの読み込み
	/// </summary>
	void LoadTexture(const std::string& filePath);

	// SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);

	// テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);

private://静的メンバ変数

	static TextureManager* instance;

	// コンストラクタ
	TextureManager() = default;

	// デストラクタ
	~TextureManager() = default;

	TextureManager(TextureManager&) = delete;

	TextureManager& operator=(TextureManager&) = delete;

private:

	//　テクスチャ1枚分のデータ
	struct TextureData
	{
		std::string filePath;                            // ファイルパス
		DirectX::TexMetadata metadata;                   // テクスチャメタデータ
		Microsoft::WRL::ComPtr<ID3D12Resource> resource; // テクスチャリソース
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;		 // SRVハンドル(CPU)
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;		 // SRVハンドル(GPU)
	};


	// テクスチャデータ
	std::vector<TextureData> textureDatas;

	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;


	DirectXCommon* dxCommon_ = nullptr;
};

