#include<Windows.h>

#include <string>
#include <format>
#include<dbghelp.h>
#include <strsafe.h>
#include<dxgidebug.h>
#include"Math.h"
#include"externals/DirectXTex/d3dx12.h"
#include<vector>
#include <numbers>
#include <iomanip>
#include <sstream>
#include<wrl.h>
#include <functional>
#include<array>
#include<xaudio2.h>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib,"dxcompiler.lib")

#pragma comment(lib,"xaudio2.lib")


#include "externals/DirectXTex/DirectXTex.h"


#include "Input.h"
#include "WinApp.h"
#include <cstdint>

#include "DirectXCommon.h"

Math math;

struct Transform
{

	Vector3 scale;		//スケール
	Vector3 rotate;		//回転
	Vector3 translate;	//位置
};

struct VertexData
{

	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;

	bool operator==(const VertexData& other) const
	{
		return position.x == other.position.x && position.y == other.position.y && position.z == other.position.z &&
			texcoord.x == other.texcoord.x && texcoord.y == other.texcoord.y &&
			normal.x == other.normal.x && normal.y == other.normal.y && normal.z == other.normal.z;
	}
};

struct Material
{
	Vector4 color;
	bool enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
	int32_t selectLightings;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};


struct DirectionlLight
{
	Vector4 color; // ライトの色
	Vector3 direction; //ライトの向き
	float intensity; //輝度
};

struct MaterialData
{
	std::string textureFilePath;
};

struct  ModelData
{
	std::vector<VertexData> vertices;
	std::vector<uint32_t> indices;
	MaterialData material;
};

namespace std
{
	template <>
	struct hash<VertexData>
	{
		size_t operator()(const VertexData& v) const
		{
			size_t h1 = hash<float>()(v.position.x) ^ hash<float>()(v.position.y) ^ hash<float>()(v.position.z);
			size_t h2 = hash<float>()(v.texcoord.x) ^ hash<float>()(v.texcoord.y);
			size_t h3 = hash<float>()(v.normal.x) ^ hash<float>()(v.normal.y) ^ hash<float>()(v.normal.z);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}

//チャンクヘッダ
struct ChunkHeader
{
	//チャンク前のID
	char id[4];

	//チャンクサイズ
	int32_t size;
};

//Riffヘッダーチャンク
struct RiffHeader
{
	//RIFF
	ChunkHeader chunk;
	//WAVE
	char type[4];
};

//FMTチャンク
struct FormatChunk
{
	//fmt
	ChunkHeader chunk;
	//波形フォーマット
	WAVEFORMATEX fmt;
};

//音声データ
struct SoundData
{
	//波形フォーマット
	WAVEFORMATEX wfex;

	//バッファの先頭アドレス
	BYTE* pBuffer;

	//バッファのサイズ
	unsigned int buffersize;
};



struct D3DResourceLeakChecker
{
	~D3DResourceLeakChecker()
	{
		//リソースリークチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug))))
		{
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

//void Log(std::ostream& os, const std::string& message);
//
//
//LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
//
//static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);
//
//IDxcBlob* compileshader(const std::wstring& filePath,
//	const wchar_t* profile,
//	IDxcUtils* dxcytils,
//	IDxcCompiler3* dxCompiler,
//	IDxcIncludeHandler* includeHandler,
//	std::ostream& os);
//
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInbytes);
//
//
//DirectX::ScratchImage LoadTexture(const std::string& filePath);
//
//void CreateWhiteTexture(DirectX::ScratchImage& outImage);
//
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metdata);
//
//Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages,
//	const Microsoft::WRL::ComPtr<ID3D12Device>& device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
//
//Microsoft::WRL::ComPtr<ID3D12Resource> CreatDepthStencilTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, int32_t width, int32_t height);
//
//D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);
//
//D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);
//
//ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);
//
//MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
//
//SoundData SoundLoadWave(const char* filename);
//
////音声データ解放
//void SoundUnload(SoundData* soundData);
//
////音声再生
//void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);

//Transformの初期化
Transform  transformSphere
{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

Transform  transformPlaneObj
{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

Transform  transformMultiMeshObj
{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

Transform  transformTeapotObj
{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

Transform  transformSuzanneObj
{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

Transform  transformBunnyObj
{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

//CPUで動かす用のTransform
Transform transformSprite
{
	{1.0f,1.0f,1.0f},
	{0.0f,0.0f,0.0f},
	{0.0f,0.0f,0.0f},
};

//カメラの位置
Transform camera
{
	{ 1.0f, 1.0f,  1.0f },
	{ 0.0f, 0.0f,  0.0f },
	{ 0.0f, 0.0f, -10.0f }
};

//UVTransformの初期化
Transform uvTransformSprite
{
	{ 1.0f, 1.0f, 1.0f },
	{ 0.0f, 0.0f, 0.0f },
	{ 0.0f, 0.0f, 0.0f }
};

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	D3DResourceLeakChecker leakcheck;


	//COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	////例外発生時にコールバックする関数を指定
	//SetUnhandledExceptionFilter(ExportDump);



	/*---ウィンドウクラスの登録---*/
	//ポインタ
	WinApp* winApp = nullptr;

	//windowsAPIの初期化
	winApp = new WinApp();
	winApp->Initialize();

#ifdef _DEBUG

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		//デバックレイヤーの有効化
		debugController->EnableDebugLayer();
		//GPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}

#endif 

	//ポインタ
	DirectXCommon* dxCommon = nullptr;

	//DirectX初期化
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	//キー入力の初期化

	//ポインタ
	Input* input = nullptr;

	//入力の初期化
	input = new Input();
	input->Initialize(winApp);




	////RootSignature作成
	//D3D12_ROOT_SIGNATURE_DESC descripitionRootSignature{};
	//descripitionRootSignature.Flags =
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	///*---RootSignature作成---*/
	//D3D12_ROOT_PARAMETER rootPrameters[4] = {};
	////CBVを使う
	//rootPrameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	////prixelShederを使う
	//rootPrameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	////レジスタ番号0とバインド
	//rootPrameters[0].Descriptor.ShaderRegister = 0;

	////CBVを使う
	//rootPrameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	////VertexShederを使う
	//rootPrameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	////レジスタ番号0とバインド
	//rootPrameters[1].Descriptor.ShaderRegister = 0;

	////DescriptorTableを使う
	//rootPrameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	////PixelShaderで使う
	//rootPrameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	////Tableの中身の配列を指定
	//rootPrameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	////Tableで利用する数
	//rootPrameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	////CBVを使う
	//rootPrameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	////Pixelshaderで使う
	//rootPrameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	////レジスタ番号1を使う
	//rootPrameters[3].Descriptor.ShaderRegister = 1;

	////ルートパラメータ配列へのポインタ
	//descripitionRootSignature.pParameters = rootPrameters;
	////配列の長さ
	//descripitionRootSignature.NumParameters = _countof(rootPrameters);


	//D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
	////バイリニアフィルタ
	//staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	////0~1の範囲外をリピート
	//staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	////比較しない
	//staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	////ありったけのMipMapを使う
	//staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	////レジスタ番号0を使う
	//staticSamplers[0].ShaderRegister = 0;
	////PixelShaderで使う
	//staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//descripitionRootSignature.pStaticSamplers = staticSamplers;
	//descripitionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	////シリアライズしてバイナリにする
	//Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	//Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	//hr = D3D12SerializeRootSignature(&descripitionRootSignature,
	//	D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	//if (FAILED(hr))
	//{
	//	/*Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));*/
	//	assert(false);
	//}

	////バイナリを元に生成
	//Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	//hr = device->CreateRootSignature(0,
	//	signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
	//	IID_PPV_ARGS(&rootSignature));
	//assert(SUCCEEDED(hr));

	////inputLayout
	//D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	//inputElementDescs[0].SemanticName = "POSITION";
	//inputElementDescs[0].SemanticIndex = 0;
	//inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	//inputElementDescs[1].SemanticName = "TEXCOORD";
	//inputElementDescs[1].SemanticIndex = 0;
	//inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	//inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	//inputElementDescs[2].SemanticName = "NORMAL";
	//inputElementDescs[2].SemanticIndex = 0;
	//inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	//inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	//D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	//inputLayoutDesc.pInputElementDescs = inputElementDescs;
	//inputLayoutDesc.NumElements = _countof(inputElementDescs);

	////BlenderStateの設定
	//D3D12_BLEND_DESC blendDesc{};
	////全ての色要素を書き込む
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	////ブレンディングを有効化
	//blendDesc.RenderTarget[0].BlendEnable = true;
	//blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	//blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	//blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	//blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;


	////RasiterZerStateの設定
	//D3D12_RASTERIZER_DESC rasterizerDesc{};

	////裏面を表示しない
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	////三角形の中を塗りつぶす
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	////shaderをコンパイルする
	//Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = compileshader(L"object3D.VS.hlsl",
	//	L"vs_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
	//assert(vertexShaderBlob != nullptr);

	//Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = compileshader(L"object3D.PS.hlsl",
	//	L"ps_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
	//assert(pixelShaderBlob != nullptr);

	////PSOを生成
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};

	////RootSignature
	//graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();

	////InputLayout
	//graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;

	////BlenderState
	//graphicsPipelineStateDesc.BlendState = blendDesc;

	////RasterizerState
	//graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	////VertexShader
	//graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	//vertexShaderBlob->GetBufferSize() };

	////PixelShader
	//graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	//pixelShaderBlob->GetBufferSize() };

	////DepthStencilStateの設定
	//D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	////機能を有効化
	//depthStencilDesc.DepthEnable = true;
	////書き込み
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

	//depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	////DepthStencilの設定
	//graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	//graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	////書き込むRTVの情報
	//graphicsPipelineStateDesc.NumRenderTargets = 1;
	//graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	////利用する形状のタイプ。三角形
	//graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	////どのように画面に色を打ち込むかの設定
	//graphicsPipelineStateDesc.SampleDesc.Count = 1;
	//graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	////実際に作成
	//Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;
	//hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
	//	IID_PPV_ARGS(&graphicsPipelineState));
	//assert(SUCCEEDED(hr));

	////===================================
	////球体の設定
	////===================================

	///*--- VertexBufferの設定 ---*/

	////分裂数
	//const uint32_t kSubdivision = 16;

	////経度分割1つ分の角度
	//const float kLonEvery = 2.0f * std::numbers::pi_v<float> / float(kSubdivision);

	////緯度分割1つ分の角度
	//const float kLatEvery = std::numbers::pi_v<float> / float(kSubdivision);

	////必要な頂点数
	//const uint32_t vertexCount = (kSubdivision + 1) * (kSubdivision + 1);

	////頂点リソースを作成
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSphere = CreateBufferResource(device, sizeof(VertexData) * vertexCount);

	////頂点バッファビューを作成
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};

	//////リソースの先頭のアドレスから使う
	//vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();

	////使用するリソースのサイズは頂点3つのサイズ
	//vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * vertexCount;
	////1頂点当たりのサイズ
	//vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);

	////頂点リソースデータを書き込む
	//VertexData* vertexDataSphere = nullptr;

	////書き込む為のアドレスを取得
	//vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));

	////緯度の方向に分割 -π/2 ~ π/2
	//for (uint32_t latIndex = 0; latIndex <= kSubdivision; latIndex++)
	//{
	//	//経度の方向に分割 0 ~ 2π
	//	for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; lonIndex++)
	//	{
	//		uint32_t index = latIndex * (kSubdivision + 1) + lonIndex;

	//		//現在の経度
	//		float lon = lonIndex * kLonEvery;
	//		//現在の緯度
	//		float lat = -std::numbers::pi_v<float> / 2.0f + kLatEvery * latIndex;

	//		//頂点にデータを入力する

	//		//位置
	//		vertexDataSphere[index].position.x = cos(lat) * cos(lon);
	//		vertexDataSphere[index].position.y = sin(lat);
	//		vertexDataSphere[index].position.z = cos(lat) * sin(lon);
	//		vertexDataSphere[index].position.w = 1.0f;

	//		//テクスチャ座標
	//		vertexDataSphere[index].texcoord = {
	//			 1.0f - float(lonIndex) / float(kSubdivision),
	//			 1.0f - float(latIndex) / float(kSubdivision)
	//		};

	//		//法線
	//		Vector3 normal = math.Normalize({ vertexDataSphere[index].position.x,
	//			vertexDataSphere[index].position.y, vertexDataSphere[index].position.z });
	//		vertexDataSphere[index].normal = normal;
	//	}
	//}


	///*--- 頂点インデックスの設定 ---*/

	////必要な頂点数
	//const uint32_t indexCount = kSubdivision * kSubdivision * 6;

	////球体用のインデックスリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSphere = CreateBufferResource(device, sizeof(uint32_t) * indexCount);

	////インデックスバッファビューを作成
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};

	////リソースの先頭アドレスから使う
	//indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();

	////使用するリソースのサイズはインデックス6つ分のサイズ
	//indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * indexCount;

	////インデックスはuint32_tとする
	//indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT;

	////インデックスデータの設定
	//uint32_t* indexDataSphere = nullptr;
	//indexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSphere));

	////インデックスの設定
	//uint32_t currentIndex = 0;

	////緯度の方向に分割 -π/2 ~ π/2
	//for (uint32_t latIndex = 0; latIndex < kSubdivision; latIndex++)
	//{
	//	//経度の方向に分割 0 ~ 2π
	//	for (uint32_t lonIndex = 0; lonIndex < kSubdivision; lonIndex++)
	//	{
	//		// 左上
	//		uint32_t v0 = latIndex * (kSubdivision + 1) + lonIndex;
	//		// 右上
	//		uint32_t v1 = v0 + 1;
	//		// 左下
	//		uint32_t v2 = (latIndex + 1) * (kSubdivision + 1) + lonIndex;
	//		// 右下
	//		uint32_t v3 = v2 + 1;

	//		//頂点にデータを入力する

	//		//三角形1つ目:abc
	//		indexDataSphere[currentIndex++] = v0;
	//		indexDataSphere[currentIndex++] = v1;
	//		indexDataSphere[currentIndex++] = v2;


	//		//三角形2つ目:cbd
	//		indexDataSphere[currentIndex++] = v2;
	//		indexDataSphere[currentIndex++] = v1;
	//		indexDataSphere[currentIndex++] = v3;

	//	}
	//}





	////TransformationMatrix用のリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource>transformationMatrixResourceSphere = CreateBufferResource(device, sizeof(TransformationMatrix));

	////データを書き込む
	//TransformationMatrix* transformationMatrixDataSphere = nullptr;

	////書き込むためのアドレス取得
	//transformationMatrixResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSphere));

	////単位行列を書き込む
	//transformationMatrixDataSphere->WVP = math.MakeIdentity();




	////マテリアル用のリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSphere = CreateBufferResource(device, sizeof(Material));

	////頂点リソースデータを書き込む
	//Material* materialDataSphere = nullptr;

	////書き込む為のアドレスを取得
	//materialResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSphere));

	////色の設定
	//materialDataSphere->color = { 1.0f,1.0f,1.0f,1.0f };

	////Lightingを有効化
	//materialDataSphere->enableLighting = true;

	////Lightingの種類の設定
	//materialDataSphere->selectLightings = 2;

	////単位行列を書き込む
	//materialDataSphere->uvTransform = math.MakeIdentity();


	///*-------------- オブジェクトファイル --------------*/

	////モデルの読み込み(Plane.ogj)
	//ModelData modelDataPlaneObj = LoadObjFile("resources", "plane.obj");

	////頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourcePlaneObj = CreateBufferResource(device, sizeof(VertexData) * modelDataPlaneObj.vertices.size());

	////頂点バッファビューを作成
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewPlaneObj{};

	////リソースの先頭からアドレスから使う
	//vertexBufferViewPlaneObj.BufferLocation = vertexResourcePlaneObj->GetGPUVirtualAddress();
	////使用するリソースのサイズは頂点サイズ
	//vertexBufferViewPlaneObj.SizeInBytes = UINT(sizeof(VertexData) * modelDataPlaneObj.vertices.size());
	////1頂点あたりのサイズ
	//vertexBufferViewPlaneObj.StrideInBytes = sizeof(VertexData);

	////頂点リソースにデータを書き込む
	//VertexData* vertexDataPlaneObj = nullptr;
	//vertexResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataPlaneObj));
	//std::memcpy(vertexDataPlaneObj, modelDataPlaneObj.vertices.data(), sizeof(VertexData) * modelDataPlaneObj.vertices.size());

	///*--- インデックスバッファ用リソースを作る---*/
	////頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResourcePlaneObj = CreateBufferResource(device, sizeof(uint32_t) * modelDataPlaneObj.indices.size());

	////頂点バッファビューを作成
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewPlaneObj{};

	////リソースの先頭からアドレスから使う
	//indexBufferViewPlaneObj.BufferLocation = indexResourcePlaneObj->GetGPUVirtualAddress();

	////使用するリソースのサイズは頂点サイズ
	//indexBufferViewPlaneObj.SizeInBytes = UINT(sizeof(uint32_t) * modelDataPlaneObj.indices.size());

	////1頂点あたりのサイズ
	//indexBufferViewPlaneObj.Format = DXGI_FORMAT_R32_UINT;

	////頂点リソースにデータを書き込む
	//uint32_t* indexDataPlaneObj = nullptr;
	//indexResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&indexDataPlaneObj));
	//std::memcpy(indexDataPlaneObj, modelDataPlaneObj.indices.data(), sizeof(uint32_t) * modelDataPlaneObj.indices.size());
	//indexResourcePlaneObj->Unmap(0, nullptr);


	////WVP用のリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourcePlaneObj = CreateBufferResource(device, sizeof(TransformationMatrix));

	////データを書き込む
	//TransformationMatrix* transformationMatrixDataPlaneObj = nullptr;

	////書き込むためのアドレス取得
	//transformationMatrixResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataPlaneObj));

	////単位行列を書き込む
	//transformationMatrixDataPlaneObj->WVP = math.MakeIdentity();



	////マテリアル用のリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourcePlaneObj = CreateBufferResource(device, sizeof(Material));

	////頂点リソースデータを書き込む
	//Material* materialDataPlaneObj = nullptr;

	////書き込む為のアドレスを取得
	//materialResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&materialDataPlaneObj));

	////色の設定
	//materialDataPlaneObj->color = { 1.0f,1.0f,1.0f,1.0f };

	////Lightingを有効化
	//materialDataPlaneObj->enableLighting = true;

	////Lightingの種類の設定
	//materialDataPlaneObj->selectLightings = 2;

	////単位行列を書き込む
	//materialDataPlaneObj->uvTransform = math.MakeIdentity();

	///*-------------- Spriteの設定 --------------*/

	////Sprite用の頂点リソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 4);
	////頂点バッファビューを作成
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferviewSprite{};
	////リソースの先頭アドレスから使う
	//vertexBufferviewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	////使用するリソースのサイズは頂点6つ分のサイズ
	//vertexBufferviewSprite.SizeInBytes = sizeof(VertexData) * 4;
	////1頂点あたりのサイズ
	//vertexBufferviewSprite.StrideInBytes = sizeof(VertexData);


	////頂点データの設定
	//VertexData* vertexDataSprite = nullptr;
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	////1枚目の三角形

	////左下
	//vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	//vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };

	////左上
	//vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	//vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };

	////右下
	//vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f };
	//vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	//vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };

	////右上
	//vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f };
	//vertexDataSprite[3].texcoord = { 1.0f,0.0f };
	//vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };


	////Sprite用のインデックスリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);
	////インデックスバッファビューを作成
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	////リソースの先頭アドレスから使う
	//indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	////使用するリソースのサイズはインデックス6つ分のサイズ
	//indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	////インデックスはuint32_tとする
	//indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;

	////インデックスデータの設定
	//uint32_t* indexDataSprite = nullptr;
	//indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));

	////インデックスの設定
	//indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
	//indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;


	////Sprite用のTransformationMatrix用のリソースを作る。matrix4x4　1つ分のサイズを用意する
	//Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(device, sizeof(TransformationMatrix));
	////データを書き込む
	//TransformationMatrix* transfomationMartixDataSprite = nullptr;
	////書き込むためのアドレスを取得
	//transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transfomationMartixDataSprite));
	////単位行列を書き込んでおく
	//transfomationMartixDataSprite->WVP = math.MakeIdentity();



	////Sprite用のリソースを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResourceSprite = CreateBufferResource(device, sizeof(Material));

	////頂点リソースデータを書き込む
	//Material* materialDataSprite = nullptr;

	////書き込む為のアドレスを取得
	//materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));

	////色の設定
	//materialDataSprite->color = { 1.0f,1.0f,1.0f,1.0f };

	////Lightingを有効化
	//materialDataSprite->enableLighting = false;

	////Lightingの種類の設定
	//materialDataSprite->selectLightings = 0;

	////単位行列を書き込む
	//materialDataSprite->uvTransform = math.MakeIdentity();

	///*-------------- 平行光源の設定 --------------*/

	////平行光源用のリソースを作成
	//Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResouerce = CreateBufferResource(device, sizeof(DirectionlLight));

	////データを書き込む
	//DirectionlLight* directionalLightData = nullptr;

	////書き込むためのアドレス取得
	//directionalLightResouerce->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	////ライトの色
	//directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	////向き
	//directionalLightData->direction = { 1.0f,0.0f,0.0f };
	////輝度
	//directionalLightData->intensity = 1.0f;


	////サウンド
	//Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	//IXAudio2MasteringVoice* masterVoice;


	////Textureの読み込み
	//DirectX::ScratchImage mipimages1 = LoadTexture("resources/uvChecker.png");
	//const DirectX::TexMetadata& metadata1 = mipimages1.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource1 = CreateTextureResource(device, metadata1);
	//Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource1 = UploadTextureData(textureResource1, mipimages1, device, commandList);

	////metDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc1{};
	//srvDesc1.Format = metadata1.format;
	//srvDesc1.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	////2Dテクスチャ
	//srvDesc1.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc1.Texture2D.MipLevels = UINT(metadata1.mipLevels);

	////SRVを作成するDescriptorHeapの場所を決める
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU1 = GetCPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 1);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU1 = GetGPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 1);

	////SRVを作成
	//device->CreateShaderResourceView(textureResource1.Get(), &srvDesc1, textureSrvHandleCPU1);



	////PlaneObjで読み込まれている画像を転送する
	////Textureの読み込み
	//DirectX::ScratchImage mipimages2 = LoadTexture(modelDataPlaneObj.material.textureFilePath);
	//const DirectX::TexMetadata& metadata2 = mipimages2.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device, metadata2);
	//Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2, mipimages2, device, commandList);

	////metDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	////2Dテクスチャ
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	////SRVを作成するDescriptorHeapの場所を決める
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 2);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, desriptorSizeSRV, 2);
	////先頭はImGuiが使っているので、その次を使う
	//textureSrvHandleCPU2.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//textureSrvHandleGPU2.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	////SRVを作成
	//device->CreateShaderResourceView(textureResource2.Get(), &srvDesc2, textureSrvHandleCPU2);


	////Xaudioエンジンのインスタンスを生成
	//result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	////マスターボイスを生成
	//result = xAudio2.Get()->CreateMasteringVoice(&masterVoice);


	////音声データの読み込み
	//SoundData soundData1 = SoundLoadWave("Resources/Alarm01.wav");

	////音声再生
	///*SoundPlayWave(xAudio2.Get(), soundData1);*/
	//SoundPlayWave(xAudio2.Get(), soundData1);

	//int32_t selectedModel = 0;

	//bool isDisplaySprite = true;

	///*---メインループ---*/


	////ゲームループ
	//while (true)
	//{
	//	//Windowsのメッセージ処理
	//	if (winApp->ProcessMessage())
	//	{
	//		//ゲームループを抜ける
	//		break;
	//	}

	//	ImGui_ImplDX12_NewFrame();
	//	ImGui_ImplWin32_NewFrame();
	//	ImGui::NewFrame();

	//	//入力の更新
	//	input->Update();

	//	if (input->PushKey(DIK_W))
	//	{
	//		transformPlaneObj.translate.y += 0.1f;
	//	}
	//	else if (input->PushKey(DIK_S))
	//	{
	//		transformPlaneObj.translate.y -= 0.01f;
	//	}

	//	if (input->PushKey(DIK_A))
	//	{
	//		transformPlaneObj.translate.x -= 0.01f;
	//	}
	//	else if (input->PushKey(DIK_D))
	//	{
	//		transformPlaneObj.translate.x += 0.01f;
	//	}


	//	/*-------------- ↓更新処理ここから↓ --------------*/

	//	/*--- スフィアの更新処理 ---*/


	//	//Transformの更新
	//	Matrix4x4 worldMatrix = math.MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
	//	Matrix4x4 cameraMatrix = math.MakeAffineMatrix(camera.scale, camera.rotate, camera.translate);
	//	Matrix4x4 viewMatrix = math.Matrix4x4Inverse(cameraMatrix);
	//	Matrix4x4 projectionMatrix = math.MakePerspectiveFovMatrix(0.45f, float(winApp->kClientWidth) / float(winApp->kClientHeight), 0.1f, 100.0f);
	//	//WVPMatrixの作成
	//	Matrix4x4 worldViewProjectionMatrix = math.Matrix4x4Multiply(worldMatrix, math.Matrix4x4Multiply(viewMatrix, projectionMatrix));
	//	transformationMatrixDataSphere->WVP = worldViewProjectionMatrix;
	//	transformationMatrixDataSphere->World = worldMatrix;

	//	/*--- Plane.objの更新処理 ---*/

	//	//Transformの更新
	//	Matrix4x4 worldMatrixPlaneObj = math.MakeAffineMatrix(transformPlaneObj.scale, transformPlaneObj.rotate, transformPlaneObj.translate);
	//	Matrix4x4 cameraMatrixPlaneObj = math.MakeAffineMatrix(camera.scale, camera.rotate, camera.translate);
	//	Matrix4x4 viewMatrixPlaneObj = math.Matrix4x4Inverse(cameraMatrixPlaneObj);
	//	Matrix4x4 projectionMatrixPlaneObj = math.MakePerspectiveFovMatrix(0.45f, float(winApp->kClientWidth) / float(winApp->kClientHeight), 0.1f, 100.0f);
	//	//WVPMatrixの作成
	//	Matrix4x4 worldViewProjectionMatrixPlaneObj = math.Matrix4x4Multiply(worldMatrixPlaneObj, math.Matrix4x4Multiply(viewMatrixPlaneObj, projectionMatrixPlaneObj));
	//	transformationMatrixDataPlaneObj->WVP = worldViewProjectionMatrixPlaneObj;
	//	transformationMatrixDataPlaneObj->World = worldMatrixPlaneObj;

	//	/*--- Spriteの更新処理 ---*/

	//	//Sprite用のWorldViewProjectmatrixを作る
	//	Matrix4x4 worldMatrixSprite = math.MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
	//	Matrix4x4 ViewMatrixSprite = math.MakeIdentity();
	//	Matrix4x4 projectionMatrixSprite = math.MakeOrthographicmatrix(0.0f, 0.0f, float(winApp->kClientWidth), float(winApp->kClientHeight), 0.0f, 100.0f);
	//	Matrix4x4 worldViewProjectionMatrixSprite = math.Matrix4x4Multiply(worldMatrixSprite, math.Matrix4x4Multiply(ViewMatrixSprite, projectionMatrixSprite));
	//	transfomationMartixDataSprite->WVP = worldViewProjectionMatrixSprite;

	//	//UVTransform用の行列
	//	Matrix4x4 uvTransformMatrix = math.MakeScaleMatrix(uvTransformSprite.scale);
	//	uvTransformMatrix = math.Matrix4x4Multiply(uvTransformMatrix, math.MakeRotateZMatrix(uvTransformSprite.rotate.z));
	//	uvTransformMatrix = math.Matrix4x4Multiply(uvTransformMatrix, math.MakeTranslationMatrix(uvTransformSprite.translate));
	//	materialDataSprite->uvTransform = uvTransformMatrix;

	//	//ライトの正規化
	//	directionalLightData->direction = math.Normalize(directionalLightData->direction);

	//	const char* modelNames[] = { "Sphere","PlaneObj" };

	//	const char* enableLightings[] = { "None","Lambert","Half Lambert" };

	//	//開発用UIの処理

	//	ImGui::Begin("window");


	//	ImGui::Combo("ModelSelect", &selectedModel, modelNames, IM_ARRAYSIZE(modelNames));

	//	ImGui::Checkbox("displaySprite", &isDisplaySprite);

	//	switch (selectedModel)
	//	{
	//	case 0:
	//	/*--- Sphere.obj ---*/

	//	if (ImGui::CollapsingHeader("Sphere"))
	//	{
	//		//位置
	//		ImGui::DragFloat3("transform.translate", &transformSphere.translate.x, 0.01f);

	//		// X軸の回転
	//		ImGui::SliderAngle("transform.rotate.X", &transformSphere.rotate.x);

	//		// Y軸の回転
	//		ImGui::SliderAngle("transform.rotate.Y", &transformSphere.rotate.y);

	//		// Z軸の回転
	//		ImGui::SliderAngle("transform.rotate.Z", &transformSphere.rotate.z);

	//		//スケール
	//		ImGui::DragFloat3("transform.scale", &transformSphere.scale.x, 0.01f);

	//		//カラー変更
	//		ImGui::ColorEdit4("Color", &(materialDataSphere->color).x);

	//		//Lightingの切り替え
	//		ImGui::Combo("selectedLight", &materialDataSphere->selectLightings, enableLightings, IM_ARRAYSIZE(enableLightings));
	//	}

	//	break;
	//	case 1:

	//	/*--- Plane.obj ---*/

	//	if (ImGui::CollapsingHeader("PlaneObj"))
	//	{
	//		//位置
	//		ImGui::DragFloat3("transform.translate", &transformPlaneObj.translate.x, 0.01f);

	//		// X軸の回転
	//		ImGui::SliderAngle("transform.rotate.X", &transformPlaneObj.rotate.x);

	//		// Y軸の回転
	//		ImGui::SliderAngle("transform.rotate.Y", &transformPlaneObj.rotate.y);

	//		// Z軸の回転
	//		ImGui::SliderAngle("transform.rotate.Z", &transformPlaneObj.rotate.z);

	//		//スケール
	//		ImGui::DragFloat3("transform.scale", &transformPlaneObj.scale.x, 0.01f);

	//		//カラー変更
	//		ImGui::ColorEdit4("Color", &(materialDataPlaneObj->color).x);

	//		//ライティングするかどうか
	//		ImGui::Checkbox("enableLighting", &materialDataPlaneObj->enableLighting);

	//		//Lightingの切り替え
	//		ImGui::Combo("selectedLight", &materialDataPlaneObj->selectLightings, enableLightings, IM_ARRAYSIZE(enableLightings));
	//	}

	//	break;
	//	}


	//	// カメラ設定のグループ
	//	if (ImGui::CollapsingHeader("camera"))
	//	{
	//		//位置
	//		ImGui::DragFloat3("camera.translate", &camera.translate.x, 0.01f);

	//		// X軸の回転
	//		ImGui::SliderAngle("camera.rotate.X", &camera.rotate.x);

	//		// Y軸の回転
	//		ImGui::SliderAngle("camera.rotate.Y", &camera.rotate.y);

	//		// Z軸の回転
	//		ImGui::SliderAngle("camera.rotate.Z", &camera.rotate.z);
	//	}


	//	// ライト設定のグループ
	//	if (ImGui::CollapsingHeader("Light"))
	//	{
	//		//向き
	//		ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.01f);

	//		//カラー
	//		ImGui::ColorEdit4("LightColor", &(directionalLightData->color).x);

	//		//輝度
	//		ImGui::SliderAngle("Lightrotate.Y", &directionalLightData->intensity);
	//	}



	//	if (isDisplaySprite)
	//	{
	//		if (ImGui::CollapsingHeader("Sprite"))
	//		{
	//			//位置
	//			ImGui::DragFloat3("transformSprite.translate", &transformSprite.translate.x, 1.0f);

	//			// X軸の回転
	//			ImGui::SliderAngle("transformSprite.rotate.X", &transformSprite.rotate.x);

	//			// Y軸の回転
	//			ImGui::SliderAngle("transformSprite.rotate.Y", &transformSprite.rotate.y);

	//			// Z軸の回転
	//			ImGui::SliderAngle("transformSprite.rotate.Z", &transformSprite.rotate.z);

	//			//カラー変更
	//			ImGui::ColorEdit4("Color", &(materialDataSprite->color).x);

	//			//Lightingの切り替え
	//			ImGui::Combo("selectedLight", &materialDataSprite->selectLightings, enableLightings, IM_ARRAYSIZE(enableLightings));


	//			// スプライト変換設定のグループ
	//			if (ImGui::CollapsingHeader("Sprite Transform"))
	//			{
	//				ImGui::DragFloat3("transformSprite", &transformSprite.translate.x, 2.0f);
	//				ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
	//				ImGui::DragFloat2("UVRotate", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
	//				ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
	//			}
	//		}
	//	}





	//	ImGui::End();

	//	/*-------------- ↓描画処理ここから↓ --------------*/

	//	//ImGuiの内部コマンドを生成
	//	ImGui::Render();

	//	//これから書き込むバックバッファのインデックスを取得
	//	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	//	//TransitionBarrierの設定
	//	D3D12_RESOURCE_BARRIER barrier{};

	//	//バリアの種類(今回はTransition)
	//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

	//	//Noneにする
	//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	//	//バリアの設定(バリアを張る対象)
	//	barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();

	//	//現在のResourceState
	//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

	//	//次のResourceState
	//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	//	//TransitionBarrierを張る
	//	commandList->ResourceBarrier(1, &barrier);

	//	//描画先のRTVを設定
	//	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, nullptr);

	//	//指定した色で画面全体をクリアする
	//	//RGBAの順番で指定
	//	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	//	commandList->ClearRenderTargetView(
	//		rtvHandles[backBufferIndex], //クリアするRTV
	//		clearColor,                 //クリアする色
	//		0,                          //指定しない
	//		nullptr                     //指定しない
	//	);

	//	//描画用のDesciptorHeapを設定
	//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorheaps[] = { srvDescriptorHeap };
	//	commandList->SetDescriptorHeaps(1, descriptorheaps->GetAddressOf());

	//	//Viewportを設定
	//	commandList->RSSetViewports(1, &viewport);

	//	//Scirssorを設定
	//	commandList->RSSetScissorRects(1, &scissorRect);

	//	//RootSignatureの設定
	//	commandList->SetGraphicsRootSignature(rootSignature.Get());

	//	//PSOを設定
	//	commandList->SetPipelineState(graphicsPipelineState.Get());

	//	//形状を設定
	//	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//	//描画先のRTVとDSVを設定
	//	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//	commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);

	//	//指定した深度で画面全体をクリア
	//	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//	switch (selectedModel)
	//	{
	//	case 0:
	//	/*--- Sphere.obj ---*/

	//	//VBVの設定
	//	commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);

	//	//IBVの設定
	//	commandList->IASetIndexBuffer(&indexBufferViewSphere);

	//	//CBVの設定
	//	commandList->SetGraphicsRootConstantBufferView(0, materialResourceSphere->GetGPUVirtualAddress());

	//	//wvp用のCBufferの場所を設定
	//	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSphere->GetGPUVirtualAddress());

	//	//SRVのDescriptorTableの先頭を設定
	//	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU1);

	//	//平行光源用のCBufferの場所を設定
	//	commandList->SetGraphicsRootConstantBufferView(3, directionalLightResouerce->GetGPUVirtualAddress());

	//	//描画！
	//	commandList->DrawIndexedInstanced(indexCount, 1, 0, 0, 0);

	//	break;
	//	case 1:

	//	/*--- Plane.obj ---*/

	//	//VBVの設定
	//	commandList->IASetVertexBuffers(0, 1, &vertexBufferViewPlaneObj);

	//	//IBVの設定
	//	commandList->IASetIndexBuffer(&indexBufferViewPlaneObj);

	//	//CBVの設定
	//	commandList->SetGraphicsRootConstantBufferView(0, materialResourcePlaneObj->GetGPUVirtualAddress());

	//	//wvp用のCBufferの場所を設定
	//	commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourcePlaneObj->GetGPUVirtualAddress());

	//	//SRVのDescriptorTableの先頭を設定
	//	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU2);

	//	//平行光源用のCBufferの場所を設定
	//	commandList->SetGraphicsRootConstantBufferView(3, directionalLightResouerce->GetGPUVirtualAddress());

	//	//描画！
	//	commandList->DrawIndexedInstanced(UINT(modelDataPlaneObj.indices.size()), 1, 0, 0, 0);

	//	break;
	//	}

	//	if (isDisplaySprite)
	//	{
	//		/*--- Sprite ---*/

	//		//VBVの設定
	//		commandList->IASetVertexBuffers(0, 1, &vertexBufferviewSprite);

	//		//IBVの設定
	//		commandList->IASetIndexBuffer(&indexBufferViewSprite);

	//		//TransfomationMatrixCBufferの場所を指定
	//		commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());

	//		//マテリアルCBufferの場所を設定
	//		commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());

	//		//SRVのDescriptorTableの先頭を設定
	//		commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU1);

	//		//平行光源用のCBufferの場所を設定
	//		commandList->SetGraphicsRootConstantBufferView(3, directionalLightResouerce->GetGPUVirtualAddress());

	//		//描画！
	//		/*commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);*/
	//	}




	//	//実際のCommandListのImGuiの描画コマンドを積む
	//	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());



	//	//状態を遷移(RenderTargetからPresentにする)
	//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	//	//TransitionBarrierを張る
	//	commandList->ResourceBarrier(1, &barrier);

	//	//コマンドリストの内容を確定させ、全てのコマンドを積んでからcloseする
	//	hr = commandList->Close();
	//	//コマンドリストの確定に失敗した場合起動できない
	//	assert(SUCCEEDED(hr));

	//	//GPUにコマンドリストを実行させる
	//	Microsoft::WRL::ComPtr<ID3D12CommandList> commandLists[] = { commandList };
	//	commandQueue->ExecuteCommandLists(1, commandLists->GetAddressOf());

	//	//GPUとOSに画面交換を行うように通知
	//	swapChain->Present(1, 0);

	//	//Fanceの値を更新
	//	fenceValue++;
	//	//GPUがここまでたどり着いた時、Fanseの値を指定した値に代入するようにsignalを送る
	//	commandQueue->Signal(fence.Get(), fenceValue);

	//	//Fanceの値が指定したSignal値たどり着いているか確認する
	//	//GetCompletedValueの初期値はFance作成時に渡した初期値
	//	if (fence->GetCompletedValue() < fenceValue)
	//	{
	//		//指定したSignal値までGPUがたどり着いていない場合、たどり着くまで待つように、イベントを設定する
	//		fence->SetEventOnCompletion(fenceValue, fenceEvent);
	//		//イベントが発火するまで待つ
	//		WaitForSingleObject(fenceEvent, INFINITE);
	//	}

	//	//次のフレーム用のコマンドリストを準備
	//	hr = commandAllocator->Reset();
	//	//コマンドアロケータのリセットに失敗した場合起動できない
	//	assert(SUCCEEDED(hr));
	//	hr = commandList->Reset(commandAllocator.Get(), nullptr);
	//	//コマンドリストのリセットに失敗した場合起動できない
	//	assert(SUCCEEDED(hr));

	//}

	////ImGuiの終了処理
	//ImGui_ImplDX12_Shutdown();
	//ImGui_ImplWin32_Shutdown();
	//ImGui::DestroyContext();


	////XAudio2の解放
	//xAudio2.Reset();

	//入力の解放
	delete input;

	//WindowsAPIの終了処理
	winApp->Finalize();

	//WindowsAPIの解放
	delete winApp;
	winApp = nullptr;

	//DirectX12の解放
	delete dxCommon;


	////音声データ解放
	//SoundUnload(&soundData1);

	////解放処理
	//CloseHandle(fenceEvent);



	return 0;
}

//---ここから下は関数の実装---//

////ウィンドウプロシージャ
//LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
//{
//
//	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
//	{
//		return true;
//	}
//
//	//メッセージに応じてゲーム固有の処理を行う
//	switch (msg)
//	{
//		//ウィンドウが破棄された
//	case WM_DESTROY:
//	//OSに対して、アプリの終了を伝える
//	PostQuitMessage(0);
//	return 0;
//	}
//
//	//標準のメッセージ処理を行う
//	return DefWindowProc(hwnd, msg, wparam, lparam);
//}
//
////出力ウィンドウにメッセージを出力する
//void Log(std::ostream& os, const std::string& message)
//{
//	os << message << std::endl;
//
//	//標準出力にメッセージを出力
//	OutputDebugStringA(message.c_str());
//}
//
//
//
//static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
//{
//	//時刻を取得して、時刻を名前に入れたファイルを作成
//	SYSTEMTIME time;
//	GetLocalTime(&time);
//
//	wchar_t filePath[MAX_PATH] = { 0 };
//	CreateDirectory(L"./Dumps", nullptr);
//	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-02d%02d-%02d%02d.dmp",
//		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
//
//	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ |
//		GENERIC_WRITE, FILE_SHARE_WRITE |
//		FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
//	//processIdとクラッシュの発生したthreadIdを取得
//	DWORD processId = GetCurrentProcessId();
//	DWORD threadId = GetCurrentThreadId();
//
//	//設定情報を入力
//	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
//	minidumpInformation.ThreadId = threadId;
//	minidumpInformation.ExceptionPointers = exception;
//	minidumpInformation.ClientPointers = true;
//
//	//Dumpを出力
//	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
//		MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
//
//
//	return EXCEPTION_EXECUTE_HANDLER;
//
//}
//
//IDxcBlob* compileshader(const std::wstring& filePath,
//	const wchar_t* profile,
//	IDxcUtils* dxcUtils,
//	IDxcCompiler3* dxcCompiler,
//	IDxcIncludeHandler* includeHandler,
//	std::ostream& os)
//{
//	/* 1.hlslファイルを読む */
//
//	//これからシェーダーのコンパイルする旨をログを出す
//	Log(os, ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n",
//		filePath, profile)));
//
//	//hlslファイルを読む
//	IDxcBlobEncoding* shaderSource = nullptr;
//	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
//
//	//読めなかったら止める
//	assert(SUCCEEDED(hr));
//
//	//読み込んだファイルの内容を設定する
//	DxcBuffer shaderSourceBuffer;
//	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
//	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
//
//	//UFT8の文字コードであることを通知
//	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
//
//	/* 2.Compileする */
//	LPCWSTR arguments[] = {
//		filePath.c_str(), //コンパイル対象のhlslファイル名
//		L"-E",L"main",    //エントリーポイントの指定。基本的にmain以外にはしない
//		L"-T",profile,    //ShaderProfileの設定
//		L"-Zi",L"-Qembed_debug", //デバック用の情報を埋め込む
//		L"-Od",  //最適化を外す
//		L"-Zpr", //メモリーレイアウトは行優先
//	};
//
//	//実際にShaderをコンパイルする
//	IDxcResult* shaderResult = nullptr;
//	hr = dxcCompiler->Compile(
//		&shaderSourceBuffer, //読み込んだファイル
//		arguments,           //コンパイルオプション
//		_countof(arguments), //コンパイルオプションの数
//		includeHandler,      //includeが含まれた諸々
//		IID_PPV_ARGS(&shaderResult)
//	);
//
//	//コンパイルエラーではなくdxcが起動できないなどの致命的な状況
//	assert(SUCCEEDED(hr));
//
//	/* 3.警告・エラーが出ていないか確認する */
//	IDxcBlobUtf8* shaderError = nullptr;
//	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
//	if (shaderError != nullptr && shaderError->GetStringLength() != 0)
//	{
//		Log(os, shaderError->GetStringPointer());
//		//警告・エラーダメ絶対
//		assert(false);
//	}
//
//	/* 4.Compile結果を受け取って返す */
//	//コンパイル結果から実行用バイナリ部分を取得
//	IDxcBlob* shaderBlob = nullptr;
//	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
//	assert(SUCCEEDED(hr));
//
//	//成功したらログを出力
//	Log(os, ConvertString(std::format(L"Compile Succeded, path:{}, profile:{}\n", filePath, profile)));
//
//	//もう使わないソースの解放
//	shaderSource->Release();
//	shaderResult->Release();
//
//	//実行用バイナリ返却
//	return shaderBlob;
//}
//
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t sizeInbytes)
//{
//
//	//頂点リソース用のヒープの設定
//	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
//
//	//UploadHeapを使う
//	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
//
//	//頂点リソースの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//
//	//バッファリソース。テクスチャの場合はまた別の設定をする
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
//	resourceDesc.Width = sizeInbytes;
//
//	//バッファの場合はこれらを1にする決まり
//	resourceDesc.Height = 1;
//	resourceDesc.DepthOrArraySize = 1;
//	resourceDesc.MipLevels = 1;
//	resourceDesc.SampleDesc.Count = 1;
//
//	//バッファの場合はこれにする決まり
//	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
//
//
//	//実際に頂点リソースを作る
//	Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource = nullptr;
//
//	HRESULT hr = device->CreateCommittedResource(
//		&uploadHeapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&resourceDesc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(&bufferResource)
//	);
//
//	assert(SUCCEEDED(hr));
//
//	return bufferResource;
//}
//
//
//DirectX::ScratchImage LoadTexture(const std::string& filePath)
//{
//
//	//テクスチャファイルを読んでプログラムで扱えるようにする
//	DirectX::ScratchImage image{};
//
//	DirectX::ScratchImage mipImages{};
//
//	std::wstring filePathW = ConvertString(filePath);
//
//	HRESULT hr = DirectX::LoadFromWICFile(
//		filePathW.c_str(),
//		DirectX::WIC_FLAGS_DEFAULT_SRGB,
//		nullptr,
//		image
//	);
//
//
//	// 読み込み失敗なら白テクスチャを返す
//	if (FAILED(hr))
//	{
//		// 白色1x1のテクスチャを作成
//		DirectX::TexMetadata metadata{};
//		metadata.width = 1;
//		metadata.height = 1;
//		metadata.arraySize = 1;
//		metadata.mipLevels = 1;
//		metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
//		metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
//
//		DirectX::Image whiteImage{};
//		whiteImage.width = 1;
//		whiteImage.height = 1;
//		whiteImage.format = DXGI_FORMAT_R8G8B8A8_UNORM;
//		whiteImage.rowPitch = 4;
//		whiteImage.slicePitch = 4;
//
//		uint8_t* pixels = new uint8_t[4]{ 255, 255, 255, 255 }; // 白 RGBA
//		whiteImage.pixels = pixels;
//
//		image.InitializeFromImage(whiteImage);
//
//		// mipなしでそのまま返す
//		mipImages.InitializeFromImage(whiteImage);
//
//		delete[] pixels;
//		return mipImages;
//	}
//
//	//ミップマップの作成
//
//	hr = DirectX::GenerateMipMaps(
//		image.GetImages(),
//		image.GetImageCount(),
//		image.GetMetadata(),
//		DirectX::TEX_FILTER_SRGB,
//		0,
//		mipImages);
//
//	assert(SUCCEEDED(hr));
//
//	//ミップマップ付きのデータを返す
//	return mipImages;
//}
//
//
//void CreateWhiteTexture(DirectX::ScratchImage& outImage)
//{
//	//白色
//	constexpr uint8_t whitePixel[4] = { 255, 255, 255, 255 };
//
//	DirectX::Image img{};
//
//	//Textureの幅
//	img.width = 1;
//	//Textureの高さ
//	img.height = 1;
//
//	img.format = DXGI_FORMAT_R8G8B8A8_UNORM;
//
//	img.pixels = const_cast<uint8_t*>(whitePixel);
//
//	img.rowPitch = 4;
//
//	img.slicePitch = 4;
//
//	outImage.InitializeFromImage(img);
//}
//
//
//Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device, const DirectX::TexMetadata& metdata)
//{
//	//1.metadataを元にResureceの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	//Textureの幅
//	resourceDesc.Width = UINT(metdata.width);
//	//Textureの高さ
//	resourceDesc.Height = UINT(metdata.height);
//	//mipmapの数
//	resourceDesc.MipLevels = UINT16(metdata.mipLevels);
//	//奥行き or　配列Textureの配列数
//	resourceDesc.DepthOrArraySize = UINT16(metdata.arraySize);
//	//TextureのFormat
//	resourceDesc.Format = metdata.format;
//	//サンプリングカウント。1固定。
//	resourceDesc.SampleDesc.Count = 1;
//	//Textrueの次元数。普段使っているのは2次元
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metdata.dimension);
//
//	//2.利用するHeapの設定
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	//細かい設定
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
//
//	//3.Resourceの生成
//	Microsoft::WRL::ComPtr<ID3D12Resource> resourece = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties,//Heapの設定
//		D3D12_HEAP_FLAG_NONE, //Heapの特殊な設定
//		&resourceDesc, //Resourceの設定
//		D3D12_RESOURCE_STATE_COPY_DEST,//データ転送される設定
//		nullptr,//Clearの最適値
//		IID_PPV_ARGS(&resourece)//生成したResourceのポインタ
//	);
//	assert(SUCCEEDED(hr));
//
//	return resourece;
//}
//
//[[nodiscard]]
//Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImages,
//	const Microsoft::WRL::ComPtr<ID3D12Device>& device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList)
//{
//	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
//	DirectX::PrepareUpload(
//		device.Get(),
//		mipImages.GetImages(),
//		mipImages.GetImageCount(),
//		mipImages.GetMetadata(),
//		subresources
//	);
//
//	uint64_t intermediatesize = GetRequiredIntermediateSize(
//		texture.Get(), 0, UINT(subresources.size())
//	);
//
//	Microsoft::WRL::ComPtr<ID3D12Resource> intermediataeResource = CreateBufferResource(device.Get(), intermediatesize);
//
//	UpdateSubresources(
//		commandList.Get(),
//		texture.Get(),
//		intermediataeResource.Get(),
//		0,
//		0,
//		UINT(subresources.size()),
//		subresources.data()
//	);
//
//	D3D12_RESOURCE_BARRIER barrier{};
//	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
//	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
//	barrier.Transition.pResource = texture.Get();
//	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
//	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
//	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
//
//	commandList->ResourceBarrier(1, &barrier);
//
//	return intermediataeResource;
//}
//
//ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)
//{
//	/*--- 1.中で必要となる変数の宣言 ---*/
//	//構成するモデルデータ
//	ModelData modelData;
//	//位置
//	std::vector<Vector4> positions;
//	//法線
//	std::vector<Vector3> normals;
//	//テクスチャ座標
//	std::vector<Vector2> texcoords;
//	//ファイルから読んだ1行を格納するもの
//	std::string line;
//
//	// 頂点データ 
//	std::unordered_map<VertexData, uint32_t> vertexToIndex;
//
//
//	/*--- 2.ファイルを開く ---*/
//	//ファイルを開く
//	std::ifstream file(directoryPath + "/" + filename);
//
//	//開かなかったら止める
//	assert(file.is_open());
//
//	/*--- 3.実際にファイルを読み、ModelDataを構築していく ---*/
//	while (std::getline(file, line))
//	{
//		std::string identfier;
//		std::istringstream s(line);
//
//		//先頭の識別子を読む
//		s >> identfier;
//
//		/* "V" : 頂点位置
//		   "vt": 頂点テクスチャ座標
//		   "vn": 頂点法線
//		   "f" : 面
//		*/
//
//		//頂点情報を読む
//		if (identfier == "v")
//		{
//			Vector4 position;
//
//			s >> position.x >> position.y >> position.z;
//
//			//位置のxを反転させ、左手座標系にする
//			position.x *= -1.0f;
//
//			position.w = 1.0f;
//
//			positions.push_back(position);
//		}
//		else if (identfier == "vt")
//		{
//			Vector2 texcoord;
//
//			s >> texcoord.x >> texcoord.y;
//
//			//テクスチャのyを反転させ、左手座標系にする
//			texcoord.y = 1.0f - texcoord.y;
//
//			texcoords.push_back(texcoord);
//		}
//		else if (identfier == "vn")
//		{
//			Vector3 normal;
//
//			s >> normal.x >> normal.y >> normal.z;
//
//			//法線のxを反転させ、左手座標系にする
//			normal.x *= -1.0f;
//
//			normals.push_back(normal);
//		}
//		else if (identfier == "f")
//		{
//			// 今から読み込む三角形の3つのインデックスを一時的に格納
//			std::array<uint32_t, 3> indices;
//
//			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex)
//			{
//				std::string VertexDefinition;
//				s >> VertexDefinition;
//
//				// 頂点定義を分解（位置 / UV / 法線）に分ける
//				std::istringstream v(VertexDefinition);
//
//				// 頂点情報（位置/UV/法線）のインデックス（OBJは1始まりなので後で-1する）
//				uint32_t posIndex = 0;
//				uint32_t texIndex = 0;
//				uint32_t normIndex = 0;
//
//				// スラッシュの位置を検索（例: 1/2/3 → 1が位置, 2がUV, 3が法線）
//				size_t firstSlash = VertexDefinition.find('/');
//				size_t secondSlash = VertexDefinition.find('/', firstSlash + 1);
//
//				if (firstSlash != std::string::npos && secondSlash != std::string::npos)
//				{
//					// フォーマット: v/vt/vn
//					std::string posStr = VertexDefinition.substr(0, firstSlash);
//					std::string texStr = VertexDefinition.substr(firstSlash + 1, secondSlash - firstSlash - 1);
//					std::string normStr = VertexDefinition.substr(secondSlash + 1);
//
//					if (!posStr.empty())
//					{
//						posIndex = std::stoi(posStr);
//					}
//
//					if (!texStr.empty())
//					{
//						texIndex = std::stoi(texStr);
//					}
//
//					if (!normStr.empty())
//					{
//						normIndex = std::stoi(normStr);
//					}
//				}
//				else if (firstSlash != std::string::npos && VertexDefinition.find("//") != std::string::npos)
//				{
//					// フォーマット: v//vn（UVなし）
//					std::string posStr = VertexDefinition.substr(0, firstSlash);
//					std::string normStr = VertexDefinition.substr(firstSlash + 2); // "//" のあと
//
//					if (!posStr.empty())
//					{
//						posIndex = std::stoi(posStr);
//					}
//
//					if (!normStr.empty())
//					{
//						normIndex = std::stoi(normStr);
//					}
//				}
//				else if (firstSlash != std::string::npos)
//				{
//					// フォーマット: v/vt（法線なし）
//					std::string posStr = VertexDefinition.substr(0, firstSlash);
//					std::string texStr = VertexDefinition.substr(firstSlash + 1);
//
//					if (!posStr.empty())
//					{
//						posIndex = std::stoi(posStr);
//					}
//
//					if (!texStr.empty())
//					{
//						texIndex = std::stoi(texStr);
//					}
//				}
//				else
//				{
//					// フォーマット: v（位置のみ）
//					posIndex = std::stoi(VertexDefinition);
//				}
//
//				// インデックスを使って頂点情報を取得
//				Vector4 position = positions[posIndex - 1];
//				Vector2 texcoord = texIndex ? texcoords[texIndex - 1] : Vector2{ 0.0f, 0.0f };
//				Vector3 normal = normIndex ? normals[normIndex - 1] : Vector3{ 0.0f, 0.0f, 0.0f };
//
//
//
//				// この3要素を1つの頂点データとしてまとめる
//				VertexData vertex{ position, texcoord, normal };
//
//				// 頂点が未登録なら新規追加、すでにあるなら再利用
//				if (vertexToIndex.count(vertex) == 0)
//				{
//					uint32_t newIndex = static_cast<uint32_t>(modelData.vertices.size());
//
//					vertexToIndex[vertex] = newIndex;
//
//					// 頂点リストに追加
//					modelData.vertices.push_back(vertex);
//				}
//
//				// 頂点に対応するインデックスを三角形インデックス配列に保存
//				indices[faceVertex] = vertexToIndex[vertex];
//			}
//
//			// 頂点の並び順を反転して左手系に対応
//			modelData.indices.push_back(indices[2]);
//			modelData.indices.push_back(indices[1]);
//			modelData.indices.push_back(indices[0]);
//
//		}
//		else if (identfier == "mtllib")
//		{
//			//materialTemplateLibraryファイルの名前を取得
//			std::string materialFilename;
//			s >> materialFilename;
//
//			//基本的にobjファイルと同一階層にmtlは存在させるので、
//			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
//
//		}
//	}
//
//	/*--- 4.Modeldataを返す ---*/
//
//	return modelData;
//}
//
//MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
//{
//	/*---	1.中で必要となる変数の宣言	---*/
//
//	//構築するMaterialData
//	MaterialData materialData;
//
//	//ファイルから読み込んだ1行を格納用
//	std::string line;
//
//	/*---	2.ファイルを開く	---*/
//
//	//ファイルを開く
//	std::ifstream file(directoryPath + "/" + filename);
//
//	//とりあえず開かなかったら止める
//	assert(file.is_open());
//
//	/*---	3.実際にファイルを読み	---*/
//
//	//ファイルを読み、MaterialDataを構築
//	while (std::getline(file, line))
//	{
//		std::string identifier;
//		std::istringstream s(line);
//
//		s >> identifier;
//
//		//identfierに応じた処理
//		if (identifier == "map_Kd")
//		{
//			std::string textureFilename;
//			s >> textureFilename;
//
//			//連結してファイルをパスにする
//			materialData.textureFilePath = directoryPath + "/" + textureFilename;
//
//		}
//	}
//
//
//	/*---	4.MaterialDataを返す	---*/
//
//	return  materialData;
//}
//
//SoundData SoundLoadWave(const char* filename)
//{
//	HRESULT result = {};
//
//	/*---　1. ファイルを開く ---*/
//	//ファイル入力ストリームのインスタンス
//	std::ifstream file;
//
//	//.wavファイルをバイナリモードで開く
//	file.open(filename, std::ios_base::binary);
//
//	//とりあえず開かなかったら止める
//	assert(file.is_open());
//
//	/*---　2. .wavデータ読み込み ---*/
//	//RIFFヘッダーの読み込み
//	RiffHeader riff;
//
//	//チャンクヘッダーの確認
//	file.read((char*)&riff, sizeof(riff));
//
//	//ファイルがRIFFかチェックする
//	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//ファイルがWAVEかチェックする
//	if (strncmp(riff.type, "WAVE", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//Formatチャンクの読み込み
//	FormatChunk format = {};
//
//	//チャンクヘッダーの確認
//	file.read((char*)&format, sizeof(ChunkHeader));
//
//	//ファイルがfmtかチェックする
//	if (strncmp(format.chunk.id, "fmt ", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//チャンク本体の読み込み
//	assert(format.chunk.size <= sizeof(format.fmt));
//	file.read((char*)&format.fmt, format.chunk.size);
//
//	//Dataチャンクの読み込み
//	ChunkHeader data;
//
//	//チャンクヘッダーの確認
//	file.read((char*)&data, sizeof(data));
//
//	//JUNKチャンクを検出した場合
//	if (strncmp(data.id, "JUNK", 4) == 0)
//	{
//		//読み取り位置をJUNKチャンクの終わりまで進める
//		file.seekg(data.size, std::ios_base::cur);
//
//		//再読み込み
//		file.read((char*)&data, sizeof(data));
//	}
//
//	if (strncmp(data.id, "data", 4) != 0)
//	{
//		assert(0);
//	}
//
//	//Dataチャンクのデータ部(波形データ)の読み込み
//	char* pBuffer = new char[data.size];
//	file.read(pBuffer, data.size);
//
//	/*---　3. ファイルを閉じる ---*/
//	//Waveファイルを閉じる
//	file.close();
//
//	/*--- 4. 読み込んだ音声データをreturnする ---*/
//	//returnするための音声データ
//	SoundData soundData = {};
//
//	//波形フォーマット
//	soundData.wfex = format.fmt;
//	//波形データ
//	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
//	//波形データのサイズ
//	soundData.buffersize = data.size;
//
//	return soundData;
//}
//
////音声データ解放
//void SoundUnload(SoundData* soundData)
//{
//	//バッファのメモリーを解放
//	delete[] soundData->pBuffer;
//
//	soundData->pBuffer = 0;
//	soundData->buffersize = 0;
//	soundData->wfex = {};
//}
//
////音声再生
//void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
//{
//	HRESULT result;
//
//	//波形フォーマットを元にSourceVoiceを生成
//	IXAudio2SourceVoice* pSourceVoice = nullptr;
//
//	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
//	assert(SUCCEEDED(result));
//
//	//再生する波形データの設定
//	XAUDIO2_BUFFER buf{};
//	buf.pAudioData = soundData.pBuffer;
//	buf.AudioBytes = soundData.buffersize;
//	buf.Flags = XAUDIO2_END_OF_STREAM;
//
//	//波形データの再生
//	result = pSourceVoice->SubmitSourceBuffer(&buf);
//	result = pSourceVoice->Start();
//}