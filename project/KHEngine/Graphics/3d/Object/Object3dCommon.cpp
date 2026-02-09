#include "Object3dCommon.h"
#include <Windows.h> // OutputDebugStringA
#include <cstdio>

void Object3dCommon::Initialize(DirectXCommon* dxCommon)
{
	dxCommon_ = dxCommon;

	// グラフィックスパイプライン生成
	CreateGraphicsPipeline();
}

void Object3dCommon::SetCommonDrawSetting()
{
	//RootSignatureの設定
	dxCommon_->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());

	//PSOを設定
	dxCommon_->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());

	//形状を設定
	dxCommon_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Object3dCommon::CreateRootSignature()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_RANGE descriptorRange[1]{};
	//0から始まる
	descriptorRange[0].BaseShaderRegister = 0;
	//数は1つ
	descriptorRange[0].NumDescriptors = 1;
	//SRVを使う
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//Offsetを自動計算
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descripitionRootSignature{};
	descripitionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	/*---RootSignature作成---*/
	D3D12_ROOT_PARAMETER rootPrameters[7] = {};

	//CBVを使う(マテリアル用)
	rootPrameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//prixelShederを使う
	rootPrameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//レジスタ番号0とバインド
	rootPrameters[0].Descriptor.ShaderRegister = 0;

	//CBVを使う(行列用)
	rootPrameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//VertexShederを使う
	rootPrameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	//レジスタ番号0とバインド
	rootPrameters[1].Descriptor.ShaderRegister = 0;

	//DescriptorTableを使う(テクスチャ用)
	rootPrameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//PixelShaderで使う
	rootPrameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//Tableの中身の配列を指定
	rootPrameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	//Tableで利用する数
	rootPrameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	//CBVを使う(ライト用)
	rootPrameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//Pixelshaderで使う
	rootPrameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//レジスタ番号1を使う
	rootPrameters[3].Descriptor.ShaderRegister = 1;

	//CBVを使う(カメラ用)
	rootPrameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//PixelShaderで使う
	rootPrameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//レジスタ番号2を使う 
	rootPrameters[4].Descriptor.ShaderRegister = 2;

	// CBVを使う(ポイントライト用)
	rootPrameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	// PixelShaderで使う
	rootPrameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	// レジスタ番号3を使う
	rootPrameters[5].Descriptor.ShaderRegister = 3;

	// CBVを使う(スポットライト用)
	rootPrameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	// PixelShaderで使う
	rootPrameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	// レジスタ番号4を使う
	rootPrameters[6].Descriptor.ShaderRegister = 4;

	//ルートパラメータ配列へのポインタ
	descripitionRootSignature.pParameters = rootPrameters;
	//配列の長さ
	descripitionRootSignature.NumParameters = _countof(rootPrameters);


	D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
	//バイリニアフィルタ
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//0~1の範囲外をリピート
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//比較しない
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	//ありったけのMipMapを使う
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	//レジスタ番号0を使う
	staticSamplers[0].ShaderRegister = 0;
	//PixelShaderで使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descripitionRootSignature.pStaticSamplers = staticSamplers;
	descripitionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&descripitionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);


	//バイナリを元に生成
	hr = dxCommon_->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
}

void Object3dCommon::CreateGraphicsPipeline()
{
	HRESULT hr;

	// ルートシグネチャ作成
	CreateRootSignature();

	//inputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//BlenderStateの設定
	D3D12_BLEND_DESC blendDesc{};
	//全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	//ブレンディングを有効化
	blendDesc.RenderTarget[0].BlendEnable = false;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;


	//RasiterZerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	//裏面を表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//shaderをコンパイルする
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxCommon_->compileshader(L"resources/shaders/object3D.VS.hlsl", L"vs_6_0");

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxCommon_->compileshader(L"resources/shaders/object3D.PS.hlsl", L"ps_6_0");

	//PSOを生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	//RootSignature
	graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();

	//InputLayout
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;

	//BlenderState
	graphicsPipelineStateDesc.BlendState = blendDesc;

	//RasterizerState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	//VertexShader
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() };

	//PixelShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//機能を有効化
	depthStencilDesc.DepthEnable = true;
	//書き込み
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//DepthStencilの設定
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//利用する形状のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//どのように画面に色を打ち込むかの設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//実際に作成
	hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));

}