#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Math/MathCommon.h"
#include "KHEngine/Core/OS/WinApp.h"
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>

class SpriteCommon;

// 頂点データ
struct vertexData
{
	Vector4 position;  //xyz：座標　w：画面外判定用
	Vector2 texcoord;  //uv：テクスチャ座標
	Vector3 normal;	   //xyz：法線
};

// マテリアルデータ
struct Material
{
	Vector4 color;		     // 色RGBA
	bool enableLighting;     // ライティング有効化フラグ
	float padding[3];	     // パディング
	Matrix4x4 uvTransform;   // UV変換行列
	int32_t selectLightings; // ライティング種類選択
};

// 座標変換行列データ
struct TransformationMatrix
{
	Matrix4x4 WVP;		// ワールドビュー射影変換行列
	Matrix4x4 World;	// ワールド変換行列
};

// スプライト
class Sprite
{
public://メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(SpriteCommon* spriteCommon, D3D12_GPU_DESCRIPTOR_HANDLE textureHandle);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// 描画処理
	///	</summary>
	void Draw();

private://メンバ関数

	/// <summary>
	///　頂点バッファ・インデックスバッファの作成
	/// </summary>
	void CreateBufferResource();

	/// <summary>
	/// マテリアルの作成
	/// </summary>
	void CreateMaterialResource();

	/// <summary>
	/// 座標変換行列データの作成
	///	</summary>
	void CreateTransformationMatrixResource();

private://メンバ変数

	// 頂点バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	// インデックスバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;


	// 頂点データの仮想アドレス
	vertexData* vertexData_ = nullptr;
	// インデックスデータの仮想アドレス
	uint32_t* indexData_ = nullptr;


	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	// インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW indexBufferView;


	// マテリアルリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;

	// マテリアルデータの仮想アドレス
	Material* materialData_ = nullptr;


	//スプライト共通部分
	SpriteCommon* spriteCommon_ = nullptr;

	// 変換行列リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;

	// 変換行列データの仮想アドレス
	TransformationMatrix* transformationMatrixData_ = nullptr;

	// テクスチャハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_;


	// DirectXCommon取得
	DirectXCommon* dxCommon = nullptr;

	WinApp* winApp_ = nullptr;
};

