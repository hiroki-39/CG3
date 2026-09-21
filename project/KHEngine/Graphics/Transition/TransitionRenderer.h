#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include "KHEngine/Math/Vector4.h"

class DirectXCommon;

class TransitionRenderer
{
public:
	// 定数バッファ用構造体 (16バイトアライメント)
	struct ConstBufferData
	{
		Vector4 fadeColor;      // トランジション色 (16 bytes)
		float progress;         // 進行度 0.0 - 1.0 (4 bytes)
		float edgeSoftness;     // 境界ぼかし幅 (4 bytes)
		int32_t isOpening;      // 0 = 閉じる(イン), 1 = 開く(アウト) (4 bytes)
		float padding0;         // パディング (4 bytes)
		Vector4 edgeColor;      // 境界ハイライト色 (16 bytes)
	};

	TransitionRenderer() = default;
	~TransitionRenderer() = default;

	void Initialize(DirectXCommon* dxCommon);
	void Draw(uint32_t ruleTextureIndex, float progress, bool isOpening,
		const Vector4& fadeColor = { 0.0f, 0.0f, 0.0f, 1.0f },
		float edgeSoftness = 0.06f,
		const Vector4& edgeColor = { 0.0f, 0.8f, 1.0f, 0.6f });

private:
	void CreateRootSignature();
	void CreateGraphicsPipeline();

private:
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
	ConstBufferData* constMap_ = nullptr;
};
