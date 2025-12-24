#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"


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

	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

	//グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
};

