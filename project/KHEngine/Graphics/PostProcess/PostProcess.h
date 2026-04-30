#pragma once
#include "KHEngine/Core/Graphics/DirectXCommon.h"

class PostProcess
{
public:
	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(DirectXCommon* dxcommon);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

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

	//SRVインデックス
	uint32_t srvIndex_;
};
