#pragma once

#include "KHEngine/Graphics/3d/Model/Model.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include <memory>
#include <string>



class Skybox
{
public:
	void Initialize(DirectXCommon* dxCommon, const std::string& cubemapTexturePath);
	void Draw(Camera* camera);

private:
	void CreateRootSignature();
	void CreatePipelineState();

	DirectXCommon* dxCommon_ = nullptr;
	//ルートシグネチャ
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;

	//グラフィックスパイプラインステート
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;

	std::unique_ptr<Model> model_;
	uint32_t cubemapSrvIndex_ = 0;

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	TransformationMatrix* transformationMatrixData_ = nullptr;
};