#pragma once

#include "KHEngine/Graphics/3d/Model/Model.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include <memory>
#include <string>



class Skybox
{
public:// 構造体

	struct TransformationMatrix
	{
		Matrix4x4 WVP;
		Matrix4x4 World;
		Matrix4x4 WorldInverseTranspose;
	};

	struct SkyboxVertexData
	{
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

public:
	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="dxCommon">dxCommon</param>
	/// <param name="cubemapTexturePath">ファイルパス</param>
	void Initialize(DirectXCommon* dxCommon, const std::string& cubemapTexturePath);
	
	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();
	
	/// <summary>
	/// 描画処理
	/// </summary>
	void Draw();


	void SetCamera(Camera* camera) { this->camera_ = camera; }

	uint32_t GetCubemapSrvIndex() const { return cubemapSrvIndex_; }

private:
	
	/// <summary>
	/// バッファリソースの作成
	/// </summary>
	void CreateBufferResource();

	/// <summary>
	/// 座標変換行列リソースの作成
	/// </summary>
	void CreateTransformationResource();

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	void CreateRootSignature();

	/// <summary>
	/// グラフィックスパイプライン生成
	/// </summary>
	void CreateGraphicsPipeline();

private:

	DirectXCommon* dxCommon = nullptr;

	Camera* camera_ = nullptr;

	uint32_t cubemapSrvIndex_ = 0;

	// 頂点バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	// 頂点バッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	// インデックスバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	// インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;

	TransformationMatrix* transformationMatrixData_ = nullptr;

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

	//グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};