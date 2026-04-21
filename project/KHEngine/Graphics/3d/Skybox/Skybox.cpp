#include "Skybox.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <Windows.h> 
#include <cstdio>


void Skybox::Initialize(DirectXCommon* dxCommon, const std::string& cubemapTexturePath)
{
	this->dxCommon = dxCommon;

	// テクスチャを読み込む
	TextureManager::GetInstance()->LoadTexture(cubemapTexturePath);
	cubemapSrvIndex_ = TextureManager::GetInstance()->GetTextureIndexByFilePath(cubemapTexturePath);

	// ルートシグネチャ作成
	CreateRootSignature();

	// グラフィックスパイプライン生成
	CreateGraphicsPipeline();

	// バッファ作成
	CreateBufferResource();

	CreateTransformationResource();
}

void Skybox::Update()
{
	if (!camera_)
	{
		return;
	}

	Matrix4x4 viewMatrix = camera_->GetViewMatrix();
	viewMatrix.m[3][0] = 0.0f; // カメラの位置を無視
	viewMatrix.m[3][1] = 0.0f;
	viewMatrix.m[3][2] = 0.0f;
	Matrix4x4 wvpMatrix = Matrix4x4::Multiply(Matrix4x4::Scale({ 500.0f,500.0f,500.0f }), viewMatrix) * camera_->GetProjectionMatrix();
	transformationMatrixData_->WVP = wvpMatrix;
	transformationMatrixData_->World = Matrix4x4::Identity();
}
void Skybox::Draw()
{
	// ルートシグネチャとパイプラインステートをセット
	dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
	dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());

	// トポロジー（形状）を設定 (これを忘れると三角形が描画されません)
	dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//VBVの設定
	dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

	//IBVの設定
	dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	//CBVの設定
	dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, transformationResource_->GetGPUVirtualAddress());

	//SRVのDescriptorTableの先頭を設定
	SrvManager::GetInstance()->SetGraphicsRootDescriptorTable(1, cubemapSrvIndex_);

	//描画！
	dxCommon->GetCommandList()->DrawIndexedInstanced(36, 1, 0, 0, 0);
}
void Skybox::CreateBufferResource()
{
	/*--- 頂点バッファ用リソースを作る ---*/
	SkyboxVertexData verteies[24] = {};
	//右面
	verteies[0].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	verteies[1].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	verteies[2].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	verteies[3].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	//左面
	verteies[4].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	verteies[5].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	verteies[6].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	verteies[7].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	//前面
	verteies[8].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	verteies[9].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	verteies[10].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	verteies[11].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	//後面
	verteies[12].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	verteies[13].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	verteies[14].position = { 1.0f, -1.0f, -1.0f, 1.0f };
	verteies[15].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	//上面
	verteies[16].position = { -1.0f, 1.0f, -1.0f, 1.0f };
	verteies[17].position = { 1.0f, 1.0f, -1.0f, 1.0f };
	verteies[18].position = { -1.0f, 1.0f, 1.0f, 1.0f };
	verteies[19].position = { 1.0f, 1.0f, 1.0f, 1.0f };
	//下面
	verteies[20].position = { -1.0f, -1.0f, 1.0f, 1.0f };
	verteies[21].position = { 1.0f, -1.0f, 1.0f, 1.0f };
	verteies[22].position = { -1.0f, -1.0f, -1.0f, 1.0f };
	verteies[23].position = { 1.0f, -1.0f, -1.0f, 1.0f };

	for (int i = 0; i < 24; ++i)
	{
		verteies[i].texcoord = { 0.0f, 0.0f };
		verteies[i].normal = { 0.0f, 0.0f, 0.0f };
	}

	//頂点リソースを作る
	vertexResource_ = dxCommon->CreateBufferResource(sizeof(verteies));

	//リソースの先頭からアドレスから使う
	vertexBufferView.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点サイズ
	vertexBufferView.SizeInBytes = UINT(sizeof(verteies));
	//1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(SkyboxVertexData);

	SkyboxVertexData* verteiesData = nullptr;

	//頂点リソースにデータを書き込む
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&verteiesData));
	std::memcpy(verteiesData, verteies, sizeof(verteies));
	vertexResource_->Unmap(0, nullptr);

	/*--- インデックスバッファ用リソースを作る ---*/
	uint32_t indices[36] = {
		0, 1, 2, 2, 1, 3, //右面
		4, 5, 6, 6, 5, 7, //左面
		8, 9,10,10, 9,11, //前面
	   12,13,14,14,13,15, //後面
	   16,17,18,18,17,19, //上面
	   20,21,22,22,21,23  //下面
	};

	size_t sizeinBytes = sizeof(indices);

	indexResource_ = dxCommon->CreateBufferResource(sizeinBytes);

	indexBufferView.BufferLocation = indexResource_->GetGPUVirtualAddress();

	indexBufferView.SizeInBytes = UINT(sizeinBytes);

	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexData_ = nullptr;

	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
	std::memcpy(indexData_, indices, sizeinBytes);
	indexResource_->Unmap(0, nullptr);
}

void Skybox::CreateTransformationResource()
{
	//WVP用のリソースを作る
	transformationResource_ = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));

	//書き込むためのアドレス取得
	transformationResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

	//単位行列を書き込む
	transformationMatrixData_->WVP = Matrix4x4::Identity();
	transformationMatrixData_->World = Matrix4x4::Identity();
	transformationMatrixData_->WorldInverseTranspose = Matrix4x4::Identity();
}

void Skybox::CreateRootSignature()
{
	HRESULT hr;

	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	//0から始まる
	descriptorRange.BaseShaderRegister = 0;
	//数は1つ
	descriptorRange.NumDescriptors = 1;
	//SRVを使う
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//Offsetを自動計算
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descripitionRootSignature{};
	descripitionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	/*---RootSignature作成---*/
	D3D12_ROOT_PARAMETER rootPrameters[2] = {};

	rootPrameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootPrameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootPrameters[0].Descriptor.ShaderRegister = 0;

	rootPrameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootPrameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootPrameters[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
	rootPrameters[1].DescriptorTable.NumDescriptorRanges = 1;

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
	hr = dxCommon->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
}

void Skybox::CreateGraphicsPipeline()
{
	HRESULT hr;

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
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

	//三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//shaderをコンパイルする
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxCommon->compileshader(L"resources/shaders/Skybox.VS.hlsl", L"vs_6_0");

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxCommon->compileshader(L"resources/shaders/Skybox.PS.hlsl", L"ps_6_0");

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
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

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
	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
}

