#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"

class Object3dCommon
{
public://メンバ関数

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxCommon);

	/// <summary>
	/// 共通描画設定
	/// </summary>
	void SetCommonDrawSetting();

	// --- Getter ---
	DirectXCommon* GetDirectXCommon() const { return dxCommon_; }
	Camera* GetDefaultCamera() const { return DefaultCamera; }

	// --- Setter ---
	void SetDefaultCamera(Camera* camera) {this->DefaultCamera = camera; }

private:

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	void CreateRootSignature();


	/// <summary>
	/// グラフィックスパイプライン生成
	/// </summary>
	void CreateGraphicsPipeline();

private:

	DirectXCommon* dxCommon_;

	Camera* DefaultCamera = nullptr;

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

	//グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};

