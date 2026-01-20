#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"

// 前方宣言: LightManager
namespace KHEngine { namespace Graphics { namespace LightSystem { class LightManager; } } }

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

	// ライトマネージャの setter/getter を追加
	void SetLightManager(KHEngine::Graphics::LightSystem::LightManager* lm) { lightManager_ = lm; }
	KHEngine::Graphics::LightSystem::LightManager* GetLightManager() const { return lightManager_; }

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

	DirectXCommon* dxCommon_ = nullptr;

	Camera* DefaultCamera = nullptr;

	// ライトマネージャ（外部で生成して渡す）
	KHEngine::Graphics::LightSystem::LightManager* lightManager_ = nullptr;

	// ライト用 GPU バッファ（shared）
	Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
	// ライト用 CPU 側マップ先
	struct DirectionalLightGPU
	{
		Vector4 color;     // 16 bytes
		Vector3 direction; // 12 bytes
		float intensity;   // 4 bytes -> 合計 32 bytes
	};
	DirectionalLightGPU* lightData_ = nullptr;

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

	//グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};

