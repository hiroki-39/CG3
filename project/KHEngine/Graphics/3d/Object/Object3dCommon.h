#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include <wrl.h>
#include <d3d12.h>
#include "KHEngine/Math/MathCommon.h"

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
	void SetDefaultCamera(Camera* camera) { this->DefaultCamera = camera; }

	// Scene-wide PointLight 操作用
	void SetPointLightColor(const Vector4& color);
	void SetPointLightPosition(const Vector3& position);
	void SetPointLightIntensity(float intensity);

private:

	/// <summary>
	/// ルートシグネチャの作成
	/// </summary>
	void CreateRootSignature();


	/// <summary>
	/// グラフィックスパイプライン生成
	/// </summary>
	void CreateGraphicsPipeline();

	/// <summary>
	/// シーン共通 PointLight 作成
	/// </summary>
	void CreateScenePointLight();

private:

	DirectXCommon* dxCommon_;

	Camera* DefaultCamera = nullptr;

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

	//グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

	// シーン共通の PointLight 用リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> scenePointLightResource_;
	// マップ先ポインタ（HLSL の PointLight レイアウトに合わせる）
	struct ScenePointLightCB
	{
		Vector4 color;
		Vector3 direction;
		float intensity;
	};
	ScenePointLightCB* scenePointLightData_ = nullptr;
};
