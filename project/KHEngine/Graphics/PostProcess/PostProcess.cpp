#include "PostProcess.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Graphics/PostProcess/Effects/GrayscaleEffect.h"
#include "KHEngine/Graphics/PostProcess/Effects/SepiaEffect.h"
#include "externals/imgui/imgui.h"
#include "externals/nlohmann/json.hpp"
#include <cassert>
#include <fstream>

void PostProcess::Initialize(DirectXCommon* dxcommon)
{
	this->dxCommon_ = dxcommon;

	CreateRootSignature();
	CreateGraphicsPipeline();

	// RenderTextureのSRVを作成
	SrvManager* srvManager = SrvManager::GetInstance();
	srvIndex_ = srvManager->Allocate();

	D3D12_RESOURCE_DESC resDesc = dxCommon_->GetRenderTextureResource()->GetDesc();
	srvManager->CreateSRVforTexture2D(srvIndex_, dxCommon_->GetRenderTextureResource().Get(), resDesc.Format, 1);

	// PostProcess先(2枚目のテクスチャ)のSRVを作成
	resultSrvIndex_ = srvManager->Allocate();
	srvManager->CreateSRVforTexture2D(resultSrvIndex_, dxCommon_->GetPostProcessTextureResource().Get(), resDesc.Format, 1);

	// 定数バッファの作成
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	D3D12_RESOURCE_DESC cbDesc{};
	cbDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbDesc.Width = (sizeof(PostProcessData) + 0xff) & ~0xff; // 256バイトアライメント
	cbDesc.Height = 1;
	cbDesc.DepthOrArraySize = 1;
	cbDesc.MipLevels = 1;
	cbDesc.SampleDesc.Count = 1;
	cbDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	dxCommon_->GetDevice()->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &cbDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&constBuffer_)
	);
	constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));

	// エフェクトの登録
	effects_.push_back(std::make_unique<GrayscaleEffect>(&data_));
	effects_.push_back(std::make_unique<SepiaEffect>(&data_));

	for (auto& effect : effects_) {
		effect->Initialize();
	}

	// 保存されている設定があればロード
	LoadFromJson();
}

void PostProcess::CreateRootSignature()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	descriptorRange[0].BaseShaderRegister = 0; // t0
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParameters[2] = {};
	// t0 (テクスチャ)
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	// b0 (定数バッファ: エフェクトのパラメータ)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[1].Descriptor.ShaderRegister = 0; // b0

	D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);
	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));

	hr = dxCommon_->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
}

void PostProcess::CreateGraphicsPipeline()
{
	HRESULT hr;

	// InputLayout (使わない)
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = nullptr;
	inputLayoutDesc.NumElements = 0;

	// BlendState
	D3D12_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaders
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->compileshader(L"resources/shaders/CopyImage.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->compileshader(L"resources/shaders/CopyImage.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// DepthStencilState (使わない)
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = false;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.BlendState = blendDesc;
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;

	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));
}

void PostProcess::Draw(bool drawToSwapchain)
{
	auto commandList = dxCommon_->GetCommandList();

	// 読み込み元(renderTexture)を RENDER_TARGET から PIXEL_SHADER_RESOURCE へ遷移
	D3D12_RESOURCE_BARRIER barriers[2]{};
	barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barriers[0].Transition.pResource = dxCommon_->GetRenderTextureResource().Get();
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	int barrierCount = 1;
	Microsoft::WRL::ComPtr<ID3D12Resource> targetResource;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;

	if (drawToSwapchain) {
		targetResource = dxCommon_->GetCurrentBackBufferResource();
		rtvHandle = dxCommon_->GetCurrentBackBufferRTVHandle();
	} else {
		targetResource = dxCommon_->GetPostProcessTextureResource();
		rtvHandle = dxCommon_->GetPostProcessRTVHandle();

		// 初回以外は PIXEL_SHADER_RESOURCE -> RENDER_TARGET へ遷移
		static bool isFirstDraw = true;
		if (!isFirstDraw) {
			barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barriers[1].Transition.pResource = targetResource.Get();
			barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; 
			barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrierCount = 2;
		}
		isFirstDraw = false;
	}

	commandList->ResourceBarrier(barrierCount, barriers);

	// 描画先のRTVを設定
	commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

	// クリア（スワップチェーンへの描画時はPreDrawSwapchainでクリア済みのため不要だが、念のため）
	if (!drawToSwapchain) {
		float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}

	// 描画設定
	commandList->SetGraphicsRootSignature(rootSignature.Get());
	commandList->SetPipelineState(graphicsPipelineState.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// エフェクトの有効/無効フラグを定数バッファに反映
	data_.enableGrayscale = 0;
	data_.enableSepia = 0;
	for (auto& effect : effects_) {
		if (effect->isActive_) {
			if (effect->name_ == "Grayscale") data_.enableGrayscale = 1;
			if (effect->name_ == "Sepia") data_.enableSepia = 1;
		}
	}

	// CPUのデータをGPUへ転送
	if (constMap_) {
		*constMap_ = data_;
	}

	// SRVセット
	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(0, srvIndex_);

	// CBVセット (b0)
	commandList->SetGraphicsRootConstantBufferView(1, constBuffer_->GetGPUVirtualAddress());

	// 描画 (3頂点)
	commandList->DrawInstanced(3, 1, 0, 0);

	// 元の状態に戻す
	barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	barrierCount = 1;
	if (!drawToSwapchain) {
		barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barriers[1].Transition.pResource = targetResource.Get();
		barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierCount = 2;
	}
	commandList->ResourceBarrier(barrierCount, barriers);

	// ★ 重要：描画先を postProcessTexture に変更していた場合、元のスワップチェーンに戻す ★
	// （これをしないと、この後の ImGui が postProcessTexture に描き込まれてしまい、SRVとの競合でクラッシュする）
	if (!drawToSwapchain) {
		D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtvHandle = dxCommon_->GetCurrentBackBufferRTVHandle();
		commandList->OMSetRenderTargets(1, &backBufferRtvHandle, false, nullptr);
	}
}

void PostProcess::DrawImGui()
{
	ImGui::Begin("Post Process Editor");

	for (auto& effect : effects_) {
		ImGui::PushID(effect->name_.c_str());
		ImGui::Checkbox(effect->name_.c_str(), &effect->isActive_);
		
		// ONの時だけ詳細なパラメータUIを表示
		if (effect->isActive_) {
			ImGui::Indent();
			effect->DrawImGui();
			ImGui::Unindent();
		}
		ImGui::PopID();
	}

	ImGui::Separator();
	if (ImGui::Button("Save Settings")) {
		SaveToJson();
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Settings")) {
		LoadFromJson();
	}

	ImGui::End();
}

void PostProcess::SaveToJson()
{
	nlohmann::json j;

	for (const auto& effect : effects_) {
		nlohmann::json effectJson;
		effectJson["isActive"] = effect->isActive_;
		
		if (effect->name_ == "Sepia") {
			effectJson["strength"] = data_.sepiaStrength;
		}

		j[effect->name_] = effectJson;
	}

	std::ofstream file("resources/json/settings/postprocess.json");
	if (file.is_open()) {
		file << j.dump(4); // インデント付きで見やすく保存
	}
}

void PostProcess::LoadFromJson()
{
	std::ifstream file("resources/json/settings/postprocess.json");
	if (!file.is_open()) return;

	nlohmann::json j;
	file >> j;

	for (auto& effect : effects_) {
		if (j.contains(effect->name_)) {
			auto& effectJson = j[effect->name_];
			
			if (effectJson.contains("isActive")) {
				effect->isActive_ = effectJson["isActive"];
			}
			
			if (effect->name_ == "Sepia" && effectJson.contains("strength")) {
				data_.sepiaStrength = effectJson["strength"];
			}
		}
	}
}
