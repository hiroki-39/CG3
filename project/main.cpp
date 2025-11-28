#include<Windows.h>
#include <string>
#include <format>
#include<dbghelp.h>
#include <strsafe.h>
#include<dxgidebug.h>
#include"externals/DirectXTex/d3dx12.h"
#include<vector>
#include <numbers>
#include <iomanip>
#include <fstream>
#include <sstream>
#include<wrl.h>
#include <functional>
#include<array>
#include<xaudio2.h>
#include <list>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib,"dxcompiler.lib")

#pragma comment(lib,"xaudio2.lib")


#include "externals/DirectXTex/DirectXTex.h"
#include "Engine/Input/Input.h"
#include "Engine/Core/OS/WinApp.h"
#include <cstdint>
#include "Engine/Core/Graphics/DirectXCommon.h"
#include <Engine/Core/Graphics/D3DResourceLeakChecker.h>
#include "Engine/Math/Math.h"

#include <random>

struct AABB
{
	Vector3 min;
	Vector3 max;
};

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
	template < >
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

enum class BlendMode
{
	Alpha = 0,
	Additive,
	Multiply,
	PreMultiplied,
	None,
	Count
};

struct  Particle
{
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float currentTime;
};

struct ParticleForGPU
{
	Matrix4x4 WVP;
	Matrix4x4 World;
	Vector4 color;
};

struct Emitter
{
	Transform transform; //エミッターの位置情報
	uint32_t count; //生成するパーティクルの数
	float frequency; //生成頻度
	float frequencyTime; //頻度用時刻
};

struct AccelerationField
{
	Vector3 accleration; //加速度
	AABB area; //範囲
};

// 追加: 表現ごとの列挙
enum class ParticleEffect
{
	Wind = 0,
	Fire,
	Snow,
	Explosion,
	Smoke,
	Confetti,
	Count
};

void Log(std::ostream& os, const std::string& message);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

void CreateWhiteTexture(DirectX::ScratchImage& outImage);

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

SoundData SoundLoadWave(const char* filename);

//音声データ解放
void SoundUnload(SoundData* soundData);

//音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);

//particle生成関数（エフェクトを指定可能に変更）
Particle MakeNewParticle(std::mt19937& randamEngine, const Vector3& translate, ParticleEffect effect = ParticleEffect::Wind);

std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randamEngine, ParticleEffect effect = ParticleEffect::Wind);

bool IsCollision(const AABB& aabb, const Vector3& point);

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	D3DResourceLeakChecker leakcheck;


	//COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	////例外発生時にコールバックする関数を指定
	//SetUnhandledExceptionFilter(ExportDump);

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

	//ポインタ
	Input* input = nullptr;

	//入力の初期化
	input = new Input();
	input->Initialize(winApp);

	HRESULT hr;

#pragma region もろもろの作成

	D3D12_DESCRIPTOR_RANGE descriptorRangeForInstancing[1]{};
	//0から始まる
	descriptorRangeForInstancing[0].BaseShaderRegister = 0;
	//数は1つ
	descriptorRangeForInstancing[0].NumDescriptors = 1;
	//SRVを使う
	descriptorRangeForInstancing[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//Offsetを自動計算
	descriptorRangeForInstancing[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE descriptorRangeTexture[1]{};
	descriptorRangeTexture[0].BaseShaderRegister = 1; // t1 (IMPORTANT)
	descriptorRangeTexture[0].NumDescriptors = 1;
	descriptorRangeTexture[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRangeTexture[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descripitionRootSignatureForInstancing{};
	descripitionRootSignatureForInstancing.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	/*---RootSignature作成---*/
	D3D12_ROOT_PARAMETER rootPrametersForInstancing[4] = {};

	rootPrametersForInstancing[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootPrametersForInstancing[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootPrametersForInstancing[0].Descriptor.ShaderRegister = 0;

	rootPrametersForInstancing[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootPrametersForInstancing[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootPrametersForInstancing[1].DescriptorTable.pDescriptorRanges = descriptorRangeForInstancing;
	rootPrametersForInstancing[1].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeForInstancing);

	rootPrametersForInstancing[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootPrametersForInstancing[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootPrametersForInstancing[2].DescriptorTable.pDescriptorRanges = descriptorRangeTexture;
	rootPrametersForInstancing[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRangeTexture);

	rootPrametersForInstancing[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootPrametersForInstancing[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootPrametersForInstancing[3].Descriptor.ShaderRegister = 1;

	//ルートパラメータ配列へのポインタ
	descripitionRootSignatureForInstancing.pParameters = rootPrametersForInstancing;
	//配列の長さ
	descripitionRootSignatureForInstancing.NumParameters = _countof(rootPrametersForInstancing);


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

	descripitionRootSignatureForInstancing.pStaticSamplers = staticSamplers;
	descripitionRootSignatureForInstancing.NumStaticSamplers = _countof(staticSamplers);

	//シリアライズしてバイナリにする
	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

	hr = D3D12SerializeRootSignature(&descripitionRootSignatureForInstancing,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

	if (FAILED(hr))
	{
		/*Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));*/
		assert(false);
	}

	//バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureForInstancing = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignatureForInstancing));
	assert(SUCCEEDED(hr));


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
	blendDesc.RenderTarget[0].BlendEnable = true;
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
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxCommon->compileshader(L"resources/shaders/Particle.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxCommon->compileshader(L"resources/shaders/Particle.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

	// PSO をブレンドモードごとに生成


	// ブレンドを生成する
	auto MakeBlendDescForMode = [](BlendMode mode)->D3D12_BLEND_DESC
		{
			D3D12_BLEND_DESC desc{};
			desc.AlphaToCoverageEnable = FALSE;
			desc.IndependentBlendEnable = FALSE;
			// デフォルト: 全て書き込む
			desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			// デフォルトは有効で、後で各モードに合わせて上書き
			switch (mode)
			{
			case BlendMode::Alpha:
			desc.RenderTarget[0].BlendEnable = TRUE;
			desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;
			case BlendMode::Additive:
			desc.RenderTarget[0].BlendEnable = TRUE;
			desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
			desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;
			case BlendMode::Multiply:
			desc.RenderTarget[0].BlendEnable = TRUE;
			// Multiply: result = src * dest
			desc.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_COLOR;
			desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
			desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;
			case BlendMode::PreMultiplied:
			desc.RenderTarget[0].BlendEnable = TRUE;
			desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
			desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
			desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;
			case BlendMode::None:
			default:
			desc.RenderTarget[0].BlendEnable = FALSE;
			desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
			desc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			break;
			}

			return desc;
		};

	// 共通なPSO記述（ブレンド以外）
	D3D12_GRAPHICS_PIPELINE_STATE_DESC commonPsoDesc{};
	commonPsoDesc.pRootSignature = rootSignatureForInstancing.Get();
	commonPsoDesc.InputLayout = inputLayoutDesc;
	commonPsoDesc.RasterizerState = rasterizerDesc;

	//DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//機能を有効化
	depthStencilDesc.DepthEnable = true;
	//書き込み
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	// DepthStencilStateの設定
	commonPsoDesc.DepthStencilState = depthStencilDesc;
	commonPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Vertex/Pixel シェーダー割当
	commonPsoDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	commonPsoDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };

	// RT/DSV 等
	commonPsoDesc.NumRenderTargets = 1;
	commonPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	commonPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	commonPsoDesc.SampleDesc.Count = 1;
	commonPsoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// PSO をブレンドモードごとに生成
	const int kBlendCount = static_cast<int>(BlendMode::Count);
	std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>> psoForBlendMode(kBlendCount);

	for (int i = 0; i < kBlendCount; ++i)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = commonPsoDesc;
		BlendMode mode = static_cast<BlendMode>(i);
		psoDesc.BlendState = MakeBlendDescForMode(mode);

		Microsoft::WRL::ComPtr<ID3D12PipelineState> pso = nullptr;
		hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
		assert(SUCCEEDED(hr));
		psoForBlendMode[i] = pso;
	}

	// 現在選択しているブレンドモード（UIで操作）
	int currentBlendModeIndex = static_cast<int>(BlendMode::Additive);
	Microsoft::WRL::ComPtr<ID3D12PipelineState> currentGraphicsPipelineState = psoForBlendMode[currentBlendModeIndex];

	/*-------------- オブジェクトファイル --------------*/

	//モデルの読み込み
	ModelData modelData;
	modelData.vertices.clear();

	modelData.vertices.push_back({ { -1.0f,  1.0f, 0.0f, 1.0f }, {0.0f, 0.0f}, {0,0,1} }); // 左上
	modelData.vertices.push_back({ {  1.0f,  1.0f, 0.0f, 1.0f }, {1.0f, 0.0f}, {0,0,1} }); // 右上
	modelData.vertices.push_back({ { -1.0f, -1.0f, 0.0f, 1.0f }, {0.0f, 1.0f}, {0,0,1} }); // 左下

	modelData.vertices.push_back({ {  1.0f,  1.0f, 0.0f, 1.0f }, {1.0f, 0.0f}, {0,0,1} }); // 右上
	modelData.vertices.push_back({ {  1.0f, -1.0f, 0.0f, 1.0f }, {1.0f, 1.0f}, {0,0,1} }); // 右下
	modelData.vertices.push_back({ { -1.0f, -1.0f, 0.0f, 1.0f }, {0.0f, 1.0f}, {0,0,1} }); // 左下
	modelData.material.textureFilePath = "resources/circle.png";


	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = dxCommon->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

	//頂点バッファビューを作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	//リソースの先頭からアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点サイズ
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	//1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	//頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	const uint32_t kNumMaxInstance = 100;

	// Instancing用のTransFormationMatrixリソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource = dxCommon->CreateBufferResource(sizeof(ParticleForGPU) * kNumMaxInstance);
	//データを書き込む
	ParticleForGPU* instancingData = nullptr;
	//書き込むためのアドレス取得
	instancingResource->Map(0, nullptr, reinterpret_cast<void**>(&instancingData));
	//単位行列を書き込む
	for (uint32_t index = 0; index < kNumMaxInstance; index++)
	{
		instancingData[index].WVP = Matrix4x4::MakeIdentity();
		instancingData[index].World = Matrix4x4::MakeIdentity();
		instancingData[index].color = { 1.0f,1.0f,1.0f,1.0f };
	}


	//マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource = dxCommon->CreateBufferResource(sizeof(Material));

	//頂点リソースデータを書き込む
	Material* materialData = nullptr;

	//書き込む為のアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	//色の設定
	materialData->color = { 1.0f,1.0f,1.0f,1.0f };

	//Lightingを有効化
	materialData->enableLighting = true;

	//Lightingの種類の設定
	materialData->selectLightings = 2;

	//単位行列を書き込む
	materialData->uvTransform = Matrix4x4::MakeIdentity();


	/*-------------- 平行光源の設定 --------------*/

	//平行光源用のリソースを作成
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResouerce = dxCommon->CreateBufferResource(sizeof(DirectionlLight));

	//データを書き込む
	DirectionlLight* directionalLightData = nullptr;

	//書き込むためのアドレス取得
	directionalLightResouerce->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));

	//ライトの色
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	//向き
	directionalLightData->direction = { 1.0f,0.0f,0.0f };
	//輝度
	directionalLightData->intensity = 1.0f;


	//Textureの読み込み
	DirectX::ScratchImage mipimages1 = dxCommon->LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metadata1 = mipimages1.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource1 = dxCommon->CreateTextureResource(dxCommon->GetDevice(), metadata1);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource1 = dxCommon->UploadTextureData(textureResource1, mipimages1);

	//metDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC instancingSrvDesc{};
	instancingSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
	instancingSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	instancingSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	instancingSrvDesc.Buffer.FirstElement = 0;
	instancingSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	instancingSrvDesc.Buffer.NumElements = kNumMaxInstance;
	instancingSrvDesc.Buffer.StructureByteStride = sizeof(ParticleForGPU);

	//SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE instancingSrvHandleCPU = dxCommon->GetSRVCPUDescriptorHandle(3);
	D3D12_GPU_DESCRIPTOR_HANDLE instancingSrvHandleGPU = dxCommon->GetSRVGPUDescriptorHandle(3);
	//SRVを作成
	dxCommon->GetDevice()->CreateShaderResourceView(instancingResource.Get(), &instancingSrvDesc, instancingSrvHandleCPU);


	D3D12_SHADER_RESOURCE_VIEW_DESC textureSrvDesc{};
	textureSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	textureSrvDesc.Format = metadata1.format;
	textureSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	textureSrvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata1.mipLevels);
	textureSrvDesc.Texture2D.MostDetailedMip = 0;
	textureSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	// SRVを作る場所（SRV番号 4 使用）
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvCPU = dxCommon->GetSRVCPUDescriptorHandle(4);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvGPU = dxCommon->GetSRVGPUDescriptorHandle(4);

	// 作成
	dxCommon->GetDevice()->CreateShaderResourceView(textureResource1.Get(), &textureSrvDesc, textureSrvCPU);

#pragma endregion

	//乱数生成器の初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);

	Emitter emitter{};
	emitter.count = 3;
	emitter.frequency = 0.5f;
	emitter.frequencyTime = 0.0f;
	emitter.transform.translate = { 0.0f,-2.0f,0.0f };
	emitter.transform.rotate = { 0.0f,0.0f,0.0f };
	emitter.transform.scale = { 1.0f,1.0f,1.0f };

	// 現在のエフェクト選択（UIで変更可能）
	ParticleEffect currentEffect = ParticleEffect::Wind;

	//パーティクルの配列（list）
	std::list<Particle> particles;
	// kNumMaxInstance * 3 のパーティクルを生成（必要なら数を調整）
	for (uint32_t index = 0; index < kNumMaxInstance; index++)
	{
		for (int j = 0; j < 3; ++j)
		{
			Particle p = MakeNewParticle(randomEngine, emitter.transform.translate, currentEffect);
			p.color = { distColor(randomEngine), distColor(randomEngine), distColor(randomEngine), 1.0f };
			p.lifeTime = distTime(randomEngine);
			p.currentTime = 0.0f;
			particles.push_back(p);
		}
	}

	const float kDeltaTime = 1.0f / 60.0f;

	//裏面回転行列
	Matrix4x4 baccktoFrontMatrix = Matrix4x4::MakeRotateYMatrix(std::numbers::pi_v<float>);

	//カメラの位置
	Transform camera
	{
		{ 1.0f, 1.0f,  1.0f },
		{ 0.0f, 0.0f,  0.0f },
		{ 0.0f, 0.0f, -20.0f }
	};

	AccelerationField accelerationField;
	accelerationField.accleration = { -15.0f, 0.0f, 0.0f };
	accelerationField.area.min = { -1.0f, -1.0f, -1.0f };
	accelerationField.area.max = { 1.0f, 1.0f, 1.0f };

	bool update = true;
	// ビルボード（カメラ目線）
	bool useBillboard = true;

	/*---メインループ---*/

	//ゲームループ
	while (true)
	{
		//Windowsのメッセージ処理
		if (winApp->ProcessMessage())
		{
			//ゲームループを抜ける
			break;
		}

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		//入力の更新
		input->Update();

		/*-------------- ↓更新処理ここから↓ --------------*/

		uint32_t numInstance = 0;

		// カメラ行列（アフィン） - ループ外で一度計算
		Matrix4x4 cameraMatrix = Matrix4x4::MakeAffine(camera.scale, camera.rotate, camera.translate);
		// ビルボード行列の計算（共通）
		Matrix4x4 billboardMatrix = Matrix4x4::MakeIdentity();

		if (useBillboard)
		{
			Matrix4x4 camRotOnly = cameraMatrix;
			camRotOnly.m[3][0] = 0.0f;
			camRotOnly.m[3][1] = 0.0f;
			camRotOnly.m[3][2] = 0.0f;
			Matrix4x4 invCam = Matrix4x4::Inverse(camRotOnly);
			billboardMatrix = baccktoFrontMatrix * invCam;
			billboardMatrix.m[3][0] = 0.0f;
			billboardMatrix.m[3][1] = 0.0f;
			billboardMatrix.m[3][2] = 0.0f;
		}
		else
		{
			billboardMatrix = Matrix4x4::MakeIdentity();
		}

		Matrix4x4 viewMatrix = Matrix4x4::Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = Matrix4x4::MakePerspectiveFov(0.45f, float(winApp->kClientWidth) / float(winApp->kClientHeight), 0.1f, 100.0f);

		//パーティクルの更新と描画準備
		for (auto it = particles.begin(); it != particles.end(); )
		{
			Particle& p = *it;

			// ライフ切れ
			if (p.currentTime >= p.lifeTime)
			{
				it = particles.erase(it);
				continue;
			}


			if (update)
			{
				//風のエフェクトの時のみ加速度を適用
				if (currentEffect == ParticleEffect::Wind)
				{
					//加速度の影響を受ける
					if (IsCollision(accelerationField.area, p.transform.translate))
					{
						p.velocity += accelerationField.accleration * kDeltaTime;
					}
				}


				//位置の更新	
				p.transform.translate += p.velocity * kDeltaTime;
				p.currentTime += kDeltaTime;
			}

			// ワールド行列を作る
			Matrix4x4 scalematrix = Matrix4x4::MakeScaleMatrix(p.transform.scale);
			Matrix4x4 translatematrix = Matrix4x4::MakeTranslationMatrix(p.transform.translate);

			Matrix4x4 worldMatrix = scalematrix * billboardMatrix * translatematrix;
			//WVPMatrixの作成
			Matrix4x4 worldViewProjectionMatrix = worldMatrix * (viewMatrix * projectionMatrix);

			if (numInstance < kNumMaxInstance)
			{
				instancingData[numInstance].WVP = worldViewProjectionMatrix;
				instancingData[numInstance].World = worldMatrix;
				instancingData[numInstance].color = p.color;
				float alpha = 1.0f - (p.currentTime / p.lifeTime);
				instancingData[numInstance].color.w = alpha;

				++numInstance;
			}

			++it;
		}

		// Emitterの更新
		emitter.frequencyTime += kDeltaTime;
		if (emitter.frequency <= emitter.frequencyTime)
		{
			particles.splice(particles.end(), Emit(emitter, randomEngine, currentEffect));
			emitter.frequencyTime -= emitter.frequency;
		}

		//開発用UIの処理

		ImGui::Begin("window");

		// エフェクト選択UI
		const char* effectItems = "Wind\0Fire\0Snow\0Explosion\0Smoke\0";
		int effectIndex = static_cast<int>(currentEffect);
		if (ImGui::Combo("Effect", &effectIndex, effectItems))
		{
			currentEffect = static_cast<ParticleEffect>(effectIndex);

			particles.clear();

			emitter.frequencyTime = 0.0f;

			switch (currentEffect)
			{
			case ParticleEffect::Wind:
			currentBlendModeIndex = static_cast<int>(BlendMode::Additive);
			emitter.count = 3;
			emitter.frequency = 0.5f;
			break;
			case ParticleEffect::Fire:
			currentBlendModeIndex = static_cast<int>(BlendMode::Additive);
			emitter.count = 6;
			emitter.frequency = 0.03f;
			break;
			case ParticleEffect::Snow:
			currentBlendModeIndex = static_cast<int>(BlendMode::Additive);
			emitter.count = 8;
			emitter.frequency = 0.02f;
			break;
			case ParticleEffect::Explosion:
			currentBlendModeIndex = static_cast<int>(BlendMode::Additive);
			emitter.count = 60;
			emitter.frequency = 1.0f;
			break;
			case ParticleEffect::Smoke:
			currentBlendModeIndex = static_cast<int>(BlendMode::Additive);
			emitter.count = 10;
			emitter.frequency = 0.1f;
			break;
			case ParticleEffect::Confetti:
			currentBlendModeIndex = static_cast<int>(BlendMode::Alpha);
			emitter.count = 20;
			emitter.frequency = 0.02f;
			break;
			default:
			currentBlendModeIndex = static_cast<int>(BlendMode::Alpha);
			break;
			}

			//　PSOを更新
			currentGraphicsPipelineState = psoForBlendMode[currentBlendModeIndex];

			// 視覚フィードバックとして新エフェクトのパーティクルを即発生させる
			if (currentEffect == ParticleEffect::Explosion)
			{
				// 爆発は一度に大量に出す（バースト）
				particles.splice(particles.end(), Emit(emitter, randomEngine, currentEffect));
			}
			else
			{
				// それ以外は少数のシードをすぐ出して切り替わりを見せる
				Emitter seed = emitter;
				// seed を少なめにしても良い（最低1）
				uint32_t seedCount = (seed.count > 3) ? (seed.count / 3) : 1;
				seed.count = seedCount;
				particles.splice(particles.end(), Emit(seed, randomEngine, currentEffect));
			}
		}

		if (ImGui::Button("Add particle"))
		{
			particles.splice(particles.end(), Emit(emitter, randomEngine, currentEffect));
		}

		ImGui::DragFloat3("EmitterTranslate", &emitter.transform.translate.x, 0.01f, -100.0f, 100.0f);
		ImGui::DragInt("EmitterCount", reinterpret_cast<int*>(&emitter.count), 1, 1, 200);
		ImGui::DragFloat("EmitterFrequency", &emitter.frequency, 0.001f, 0.0f, 10.0f);

		const char* blendItems = "Alpha\0Additive\0Multiply\0PreMultiplied\0None\0";

		if (ImGui::Combo("Blend Mode", &currentBlendModeIndex, blendItems))
		{
			// 選択が変わったら現在のPSOを更新
			currentGraphicsPipelineState = psoForBlendMode[currentBlendModeIndex];
		}

		ImGui::ColorEdit4("MaterialColor", &materialData->color.x);

		ImGui::Checkbox("UpdateParticles", &update);

		// Billboard トグルを追加
		ImGui::Checkbox("useBillboard ", &useBillboard);
		// カメラの位置
		ImGui::SliderFloat3("CameraTranslate", &camera.translate.x, -50.0f, 50.0f);
		// カメラの回転
		ImGui::SliderFloat3("CameraRotate", &camera.rotate.x, -std::numbers::pi_v<float>, std::numbers::pi_v<float>);

		ImGui::End();



		/*-------------- ↓描画処理ここから↓ --------------*/

		//ImGuiの内部コマンドを生成
		ImGui::Render();

		dxCommon->PreDraw();

		//RootSignatureの設定
		dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignatureForInstancing.Get());

		//PSOを設定
		dxCommon->GetCommandList()->SetPipelineState(currentGraphicsPipelineState.Get());

		//形状を設定
		dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//VBVの設定
		dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);

		//CBVの設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());

		//平行光源用のCBufferの場所を設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResouerce->GetGPUVirtualAddress());

		//SRVのDescriptorTableの先頭を設定
		dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(1, instancingSrvHandleGPU);

		dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvGPU);

		//描画！ (頂点数, インスタンス数, ...)
		dxCommon->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), numInstance, 0, 0);

		//実際のCommandListのImGuiの描画コマンドを積む
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

		dxCommon->PostDraw();

	}

	//ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//入力の解放
	delete input;

	//WindowsAPIの終了処理
	winApp->Finalize();

	//WindowsAPIの解放
	delete winApp;
	winApp = nullptr;

	//DirectX12の解放
	delete dxCommon;


	return 0;
}

//---ここから下は関数の実装---//

//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが破棄された
	case WM_DESTROY:
	//OSに対して、アプリの終了を伝える
	PostQuitMessage(0);
	return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//出力ウィンドウにメッセージを出力する
void Log(std::ostream& os, const std::string& message)
{
	os << message << std::endl;

	//標準出力にメッセージを出力
	OutputDebugStringA(message.c_str());
}

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception)
{
	//時刻を取得して、時刻を名前に入れたファイルを作成
	SYSTEMTIME time;
	GetLocalTime(&time);

	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-02d%02d-%02d%02d.dmp",
		time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);

	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ |
		GENERIC_WRITE, FILE_SHARE_WRITE |
		FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
	//processIdとクラッシュの発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	//設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = true;

	//Dumpを出力
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle,
		MiniDumpNormal, &minidumpInformation, nullptr, nullptr);


	return EXCEPTION_EXECUTE_HANDLER;

}

void CreateWhiteTexture(DirectX::ScratchImage& outImage)
{
	//白色
	constexpr uint8_t whitePixel[4] = { 255, 255, 255, 255 };

	DirectX::Image img{};

	//Textureの幅
	img.width = 1;
	//Textureの高さ
	img.height = 1;

	img.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	img.pixels = const_cast<uint8_t*>(whitePixel);

	img.rowPitch = 4;

	img.slicePitch = 4;

	outImage.InitializeFromImage(img);
}

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename)
{
	/*--- 1.中で必要となる変数の宣言 ---*/
	//構成するモデルデータ
	ModelData modelData;
	//位置
	std::vector<Vector4> positions;
	//法線
	std::vector<Vector3> normals;
	//テクスチャ座標
	std::vector<Vector2> texcoords;
	//ファイルから読んだ1行を格納するもの
	std::string line;

	// 頂点データ 
	std::unordered_map<VertexData, uint32_t> vertexToIndex;


	/*--- 2.ファイルを開く ---*/
	//ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);

	//開かなかったら止める
	assert(file.is_open());

	/*--- 3.実際にファイルを読み、ModelDataを構築していく ---*/
	while (std::getline(file, line))
	{
		std::string identfier;
		std::istringstream s(line);

		//先頭の識別子を読む
		s >> identfier;

		/* "V" : 頂点位置
		   "vt": 頂点テクスチャ座標
		   "vn": 頂点法線
		   "f" : 面
		*/

		//頂点情報を読む
		if (identfier == "v")
		{
			Vector4 position;

			s >> position.x >> position.y >> position.z;

			//位置のxを反転させ、左手座標系にする
			position.x *= -1.0f;

			position.w = 1.0f;

			positions.push_back(position);
		}
		else if (identfier == "vt")
		{
			Vector2 texcoord;

			s >> texcoord.x >> texcoord.y;

			//テクスチャのyを反転させ、左手座標系にする
			texcoord.y = 1.0f - texcoord.y;

			texcoords.push_back(texcoord);
		}
		else if (identfier == "vn")
		{
			Vector3 normal;

			s >> normal.x >> normal.y >> normal.z;

			//法線のxを反転させ、左手座標系にする
			normal.x *= -1.0f;

			normals.push_back(normal);
		}
		else if (identfier == "f")
		{
			// 今から読み込む三角形の3つのインデックスを一時的に格納
			std::array<uint32_t, 3> indices;

			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex)
			{
				std::string VertexDefinition;
				s >> VertexDefinition;

				// 頂点定義を分解（位置 / UV / 法線）に分ける
				std::istringstream v(VertexDefinition);

				// 頂点情報（位置/UV/法線）のインデックス（OBJは1始まりなので後で-1する）
				uint32_t posIndex = 0;
				uint32_t texIndex = 0;
				uint32_t normIndex = 0;

				// スラッシュの位置を検索（例: 1/2/3 → 1が位置, 2がUV, 3が法線）
				size_t firstSlash = VertexDefinition.find('/');
				size_t secondSlash = VertexDefinition.find('/', firstSlash + 1);

				if (firstSlash != std::string::npos && secondSlash != std::string::npos)
				{
					// フォーマット: v/vt/vn
					std::string posStr = VertexDefinition.substr(0, firstSlash);
					std::string texStr = VertexDefinition.substr(firstSlash + 1, secondSlash - firstSlash - 1);
					std::string normStr = VertexDefinition.substr(secondSlash + 1);

					if (!posStr.empty())
					{
						posIndex = std::stoi(posStr);
					}

					if (!texStr.empty())
					{
						texIndex = std::stoi(texStr);
					}

					if (!normStr.empty())
					{
						normIndex = std::stoi(normStr);
					}
				}
				else if (firstSlash != std::string::npos && VertexDefinition.find("//") != std::string::npos)
				{
					// フォーマット: v//vn（UVなし）
					std::string posStr = VertexDefinition.substr(0, firstSlash);
					std::string normStr = VertexDefinition.substr(firstSlash + 2); // "//" のあと

					if (!posStr.empty())
					{
						posIndex = std::stoi(posStr);
					}

					if (!normStr.empty())
					{
						normIndex = std::stoi(normStr);
					}
				}
				else if (firstSlash != std::string::npos)
				{
					// フォーマット: v/vt（法線なし）
					std::string posStr = VertexDefinition.substr(0, firstSlash);
					std::string texStr = VertexDefinition.substr(firstSlash + 1);

					if (!posStr.empty())
					{
						posIndex = std::stoi(posStr);
					}

					if (!texStr.empty())
					{
						texIndex = std::stoi(texStr);
					}
				}
				else
				{
					// フォーマット: v（位置のみ）
					posIndex = std::stoi(VertexDefinition);
				}

				// インデックスを使って頂点情報を取得
				Vector4 position = positions[posIndex - 1];
				Vector2 texcoord = texIndex ? texcoords[texIndex - 1] : Vector2{ 0.0f, 0.0f };
				Vector3 normal = normIndex ? normals[normIndex - 1] : Vector3{ 0.0f, 0.0f, 0.0f };


				// この3要素を1つの頂点データとしてまとめる
				VertexData vertex{ position, texcoord, normal };

				// 頂点が未登録なら新規追加、すでにあるなら再利用
				if (vertexToIndex.count(vertex) == 0)
				{
					uint32_t newIndex = static_cast<uint32_t>(modelData.vertices.size());

					vertexToIndex[vertex] = newIndex;

					// 頂点リストに追加
					modelData.vertices.push_back(vertex);
				}

				// 頂点に対応するインデックスを三角形インデックス配列に保存
				indices[faceVertex] = vertexToIndex[vertex];
			}

			// 頂点の並び順を反転して左手系に対応
			modelData.indices.push_back(indices[2]);
			modelData.indices.push_back(indices[1]);
			modelData.indices.push_back(indices[0]);

		}
		else if (identfier == "mtllib")
		{
			//materialTemplateLibraryファイルの名前を取得
			std::string materialFilename;
			s >> materialFilename;

			//基本的にobjファイルと同一階層にmtlは存在させるので、
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);

		}
	}

	/*--- 4.Modeldataを返す ---*/

	return modelData;
}

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	/*---	1.中で必要となる変数の宣言	---*/

	//構築するMaterialData
	MaterialData materialData;

	//ファイルから読み込んだ1行を格納用
	std::string line;

	/*---	2.ファイルを開く	---*/

	//ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);

	//とりあえず開かなかったら止める
	assert(file.is_open());

	/*---	3.実際にファイルを読み	---*/

	//ファイルを読み、MaterialDataを構築
	while (std::getline(file, line))
	{
		std::string identifier;
		std::istringstream s(line);

		s >> identifier;

		//identfierに応じた処理
		if (identifier == "map_Kd")
		{
			std::string textureFilename;
			s >> textureFilename;

			//連結してファイルをパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;

		}
	}


	/*---	4.MaterialDataを返す	---*/

	return  materialData;
}

SoundData SoundLoadWave(const char* filename)
{
	HRESULT result = {};

	/*---　1. ファイルを開く ---*/
	//ファイル入力ストリームのインスタンス
	std::ifstream file;

	//.wavファイルをバイナリモードで開く
	file.open(filename, std::ios_base::binary);

	//とりあえず開かなかったら止める
	assert(file.is_open());

	/*---　2. .wavデータ読み込み ---*/
	//RIFFヘッダーの読み込み
	RiffHeader riff;

	//チャンクヘッダーの確認
	file.read((char*)&riff, sizeof(riff));

	//ファイルがRIFFかチェックする
	if (strncmp(riff.chunk.id, "RIFF", 4) != 0)
	{
		assert(0);
	}

	//ファイルがWAVEかチェックする
	if (strncmp(riff.type, "WAVE", 4) != 0)
	{
		assert(0);
	}

	//Formatチャンクの読み込み
	FormatChunk format = {};

	//チャンクヘッダーの確認
	file.read((char*)&format, sizeof(ChunkHeader));

	//ファイルがfmtかチェックする
	if (strncmp(format.chunk.id, "fmt ", 4) != 0)
	{
		assert(0);
	}

	//チャンク本体の読み込み
	assert(format.chunk.size <= sizeof(format.fmt));
	file.read((char*)&format.fmt, format.chunk.size);

	//Dataチャンクの読み込み
	ChunkHeader data;

	//チャンクヘッダーの確認
	file.read((char*)&data, sizeof(data));

	//JUNKチャンクを検出した場合
	if (strncmp(data.id, "JUNK", 4) == 0)
	{
		//読み取り位置をJUNKチャンクの終わりまで進める
		file.seekg(data.size, std::ios_base::cur);

		//再読み込み
		file.read((char*)&data, sizeof(data));
	}

	if (strncmp(data.id, "data", 4) != 0)
	{
		assert(0);
	}

	//Dataチャンクのデータ部(波形データ)の読み込み
	char* pBuffer = new char[data.size];
	file.read(pBuffer, data.size);

	/*---　3. ファイルを閉じる ---*/
	//Waveファイルを閉じる
	file.close();

	/*--- 4. 読み込んだ音声データをreturnする ---*/
	//returnするための音声データ
	SoundData soundData = {};

	//波形フォーマット
	soundData.wfex = format.fmt;
	//波形データ
	soundData.pBuffer = reinterpret_cast<BYTE*>(pBuffer);
	//波形データのサイズ
	soundData.buffersize = data.size;

	return soundData;
}

//音声データ解放
void SoundUnload(SoundData* soundData)
{
	//バッファのメモリーを解放
	delete[] soundData->pBuffer;

	soundData->pBuffer = 0;
	soundData->buffersize = 0;
	soundData->wfex = {};
}

//音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData)
{
	HRESULT result;

	//波形フォーマットを元にSourceVoiceを生成
	IXAudio2SourceVoice* pSourceVoice = nullptr;

	result = xAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfex);
	assert(SUCCEEDED(result));

	//再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = soundData.pBuffer;
	buf.AudioBytes = soundData.buffersize;
	buf.Flags = XAUDIO2_END_OF_STREAM;

	//波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	result = pSourceVoice->Start();
}

// 変更: 効果ごとの初期値を与える MakeNewParticle
Particle MakeNewParticle(std::mt19937& randamEngine, const Vector3& translate, ParticleEffect effect)
{
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distUniform(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);

	Particle particle;
	particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
	particle.currentTime = 0.0f;

	switch (effect)
	{
	case ParticleEffect::Fire:
	{
		// 火: 上向きに飛び、橙〜黄色、短めの寿命、小〜中サイズ
		float rx = distUniform(randamEngine) * 0.4f;
		float rz = distUniform(randamEngine) * 0.4f;
		particle.transform.translate = translate + Vector3{ rx, 0.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 0.5f, 1.2f + dist01(randamEngine) * 1.4f, distUniform(randamEngine) * 0.5f };
		particle.color = Vector4{ 0.9f, 0.45f + dist01(randamEngine) * 0.25f, 0.05f, 1.0f };
		particle.lifeTime = 0.6f + dist01(randamEngine) * 1.2f;
		float s = 0.2f + dist01(randamEngine) * 0.8f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Snow:
	{
		// 雪: 上からゆっくり落ちる、白、長寿命、大きさゆらぎ
		float rx = distUniform(randamEngine) * 4.0f;
		float rz = distUniform(randamEngine) * 4.0f;
		particle.transform.translate = translate + Vector3{ rx, 8.0f + dist01(randamEngine) * 2.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 0.1f, -0.2f - dist01(randamEngine) * 0.6f, distUniform(randamEngine) * 0.1f };
		particle.color = Vector4{ 0.95f, 0.95f, 1.0f, 1.0f };
		particle.lifeTime = 6.0f + dist01(randamEngine) * 6.0f;
		float s = 0.15f + dist01(randamEngine) * 0.35f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Explosion:
	{
		// 爆発: 一気に飛び散る、高速、短寿命
		particle.transform.translate = translate;
		Vector3 dir = Vector3{ distUniform(randamEngine), distUniform(randamEngine), distUniform(randamEngine) };
		float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
		if (len < 1e-5f) dir = Vector3{ 1,1,0 }; else dir = dir * (1.0f / len);
		float speed = 8.0f + dist01(randamEngine) * 12.0f;
		particle.velocity = dir * speed;
		particle.color = Vector4{ 1.0f, 0.6f + dist01(randamEngine) * 0.4f, 0.1f, 1.0f };
		particle.lifeTime = 0.3f + dist01(randamEngine) * 0.7f;
		float s = 0.2f + dist01(randamEngine) * 0.6f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Smoke:
	{
		// 煙: 上昇して大きくなる、灰色、中〜長寿命
		float rx = distUniform(randamEngine) * 0.5f;
		float rz = distUniform(randamEngine) * 0.5f;
		particle.transform.translate = translate + Vector3{ rx, 0.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 0.3f, 0.4f + dist01(randamEngine) * 0.6f, distUniform(randamEngine) * 0.3f };
		float g = 0.3f + dist01(randamEngine) * 0.25f;
		particle.color = Vector4{ g, g, g, 0.8f };
		particle.lifeTime = 2.0f + dist01(randamEngine) * 4.0f;
		float s = 0.3f + dist01(randamEngine) * 1.0f;
		particle.transform.scale = Vector3{ s, s, s };
		break;
	}
	case ParticleEffect::Confetti:
	{
		// 紙吹雪: カラフルでランダムに舞う、回転は無視（見た目は色とサイズで）
		float rx = distUniform(randamEngine) * 1.0f;
		float rz = distUniform(randamEngine) * 1.0f;
		particle.transform.translate = translate + Vector3{ rx, 0.0f, rz };
		particle.velocity = Vector3{ distUniform(randamEngine) * 2.0f, 1.0f + dist01(randamEngine) * 2.0f, distUniform(randamEngine) * 2.0f };
		particle.color = Vector4{ dist01(randamEngine), dist01(randamEngine), dist01(randamEngine), 1.0f };
		particle.lifeTime = 3.0f + dist01(randamEngine) * 3.0f;
		float s = 0.05f + dist01(randamEngine) * 0.2f;
		particle.transform.scale = Vector3{ s, s * (0.4f + dist01(randamEngine) * 1.6f), s };
		break;
	}
	case ParticleEffect::Wind:
	default:
	{
		particle.transform.scale = { 1.0f,1.0f,1.0f };
		particle.transform.rotate = { 0.0f, 0.0f, 0.0f };
		particle.transform.translate = { distribution(randamEngine),distribution(randamEngine),distribution(randamEngine) };
		particle.velocity = { distribution(randamEngine), distribution(randamEngine), distribution(randamEngine) };
		particle.color = { distColor(randamEngine), distColor(randamEngine), distColor(randamEngine), 1.0f };
		particle.lifeTime = distTime(randamEngine);

		Vector3 randomTranslate{ distribution(randamEngine),
			distribution(randamEngine),
			distribution(randamEngine)
		};
		break;
	}
	}

	return particle;
}

// Emit も effect を渡すように変更
std::list<Particle> Emit(const Emitter& emitter, std::mt19937& randamEngine, ParticleEffect effect)
{
	std::list<Particle> particles;

	for (uint32_t count = 0; count < emitter.count; count++)
	{
		particles.push_back(MakeNewParticle(randamEngine, emitter.transform.translate, effect));
	}

	return particles;
}

bool IsCollision(const AABB& aabb, const Vector3& point)
{
	if (point.x < aabb.min.x || point.x > aabb.max.x) { return false; }
	if (point.y < aabb.min.y || point.y > aabb.max.y) { return false; }
	if (point.z < aabb.min.z || point.z > aabb.max.z) { return false; }
	return true;
}
