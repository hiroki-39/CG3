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
#include <fstream>
#include <sstream>
#include<wrl.h>
#include <functional>
#include<array>
#include<xaudio2.h>

#include <unordered_map>
#include <cassert>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib,"dxcompiler.lib")

#pragma comment(lib,"xaudio2.lib")


#include "externals/DirectXTex/DirectXTex.h"


#include "KHEngine/Input/Input.h"
#include "KHEngine/Core/OS/WinApp.h"
#include <cstdint>

#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Core/Graphics/D3DResourceLeakChecker.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/Resource/TextureManager.h"


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

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	D3DResourceLeakChecker leakcheck;

	//COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);

	//例外発生時にコールバックする関数を指定
	SetUnhandledExceptionFilter(ExportDump);

#pragma region 基盤システムの初期化

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

	// ポインタ
	DirectXCommon* dxCommon = nullptr;

	// DirectX初期化
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	//テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(dxCommon);

	// 入力のポインタ
	Input* input = nullptr;

	// 入力の初期化
	input = new Input();
	input->Initialize(winApp);

	// スプライトの共通部分のポインタ
	SpriteCommon* spriteCommon = nullptr;

	// スプライトの共通部分の初期化
	spriteCommon = new SpriteCommon();
	spriteCommon->Initialize(dxCommon);

#pragma endregion 

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
	D3D12_ROOT_PARAMETER rootPrameters[4] = {};
	//CBVを使う
	rootPrameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//prixelShederを使う
	rootPrameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//レジスタ番号0とバインド
	rootPrameters[0].Descriptor.ShaderRegister = 0;

	//CBVを使う
	rootPrameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//VertexShederを使う
	rootPrameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	//レジスタ番号0とバインド
	rootPrameters[1].Descriptor.ShaderRegister = 0;

	//DescriptorTableを使う
	rootPrameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//PixelShaderで使う
	rootPrameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//Tableの中身の配列を指定
	rootPrameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	//Tableで利用する数
	rootPrameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	//CBVを使う
	rootPrameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//Pixelshaderで使う
	rootPrameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//レジスタ番号1を使う
	rootPrameters[3].Descriptor.ShaderRegister = 1;

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

	if (FAILED(hr))
	{
		/*Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));*/
		assert(false);
	}

	//バイナリを元に生成
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature));
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
	Microsoft::WRL::ComPtr <IDxcBlob> vertexShaderBlob = dxCommon->compileshader(L"resources/shaders/object3D.VS.hlsl", L"vs_6_0");
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr <IDxcBlob> pixelShaderBlob = dxCommon->compileshader(L"resources/shaders/object3D.PS.hlsl", L"ps_6_0");
	assert(pixelShaderBlob != nullptr);

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
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState = nullptr;

	hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));

	assert(SUCCEEDED(hr));

	/*-------------- オブジェクトファイル --------------*/

	//モデルの読み込み(Plane.ogj)
	ModelData modelDataPlaneObj = LoadObjFile("resources", "plane.obj");

	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResourcePlaneObj = dxCommon->CreateBufferResource(sizeof(VertexData) * modelDataPlaneObj.vertices.size());

	//頂点バッファビューを作成
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewPlaneObj{};

	//リソースの先頭からアドレスから使う
	vertexBufferViewPlaneObj.BufferLocation = vertexResourcePlaneObj->GetGPUVirtualAddress();
	//使用するリソースのサイズは頂点サイズ
	vertexBufferViewPlaneObj.SizeInBytes = UINT(sizeof(VertexData) * modelDataPlaneObj.vertices.size());
	//1頂点あたりのサイズ
	vertexBufferViewPlaneObj.StrideInBytes = sizeof(VertexData);

	//頂点リソースにデータを書き込む
	VertexData* vertexDataPlaneObj = nullptr;
	vertexResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataPlaneObj));
	std::memcpy(vertexDataPlaneObj, modelDataPlaneObj.vertices.data(), sizeof(VertexData) * modelDataPlaneObj.vertices.size());

	/*--- インデックスバッファ用リソースを作る---*/
	//頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourcePlaneObj = dxCommon->CreateBufferResource(sizeof(uint32_t) * modelDataPlaneObj.indices.size());

	//頂点バッファビューを作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewPlaneObj{};

	//リソースの先頭からアドレスから使う
	indexBufferViewPlaneObj.BufferLocation = indexResourcePlaneObj->GetGPUVirtualAddress();

	//使用するリソースのサイズは頂点サイズ
	indexBufferViewPlaneObj.SizeInBytes = UINT(sizeof(uint32_t) * modelDataPlaneObj.indices.size());

	//1頂点あたりのサイズ
	indexBufferViewPlaneObj.Format = DXGI_FORMAT_R32_UINT;

	//頂点リソースにデータを書き込む
	uint32_t* indexDataPlaneObj = nullptr;
	indexResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&indexDataPlaneObj));
	std::memcpy(indexDataPlaneObj, modelDataPlaneObj.indices.data(), sizeof(uint32_t) * modelDataPlaneObj.indices.size());
	indexResourcePlaneObj->Unmap(0, nullptr);


	//WVP用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResourcePlaneObj = dxCommon->CreateBufferResource(sizeof(TransformationMatrix));

	//データを書き込む
	TransformationMatrix* transformationMatrixDataPlaneObj = nullptr;

	//書き込むためのアドレス取得
	transformationMatrixResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataPlaneObj));

	//単位行列を書き込む
	transformationMatrixDataPlaneObj->WVP = Matrix4x4::Identity();



	//マテリアル用のリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResourcePlaneObj = dxCommon->CreateBufferResource(sizeof(Material));

	//頂点リソースデータを書き込む
	Material* materialDataPlaneObj = nullptr;

	//書き込む為のアドレスを取得
	materialResourcePlaneObj->Map(0, nullptr, reinterpret_cast<void**>(&materialDataPlaneObj));

	//色の設定
	materialDataPlaneObj->color = { 1.0f,1.0f,1.0f,1.0f };

	//Lightingを有効化
	materialDataPlaneObj->enableLighting = false;

	//Lightingの種類の設定
	materialDataPlaneObj->selectLightings = 2;

	//単位行列を書き込む
	materialDataPlaneObj->uvTransform = Matrix4x4::Identity();

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


	TextureManager* texManager = TextureManager::GetInstance();

	// テクスチャアップロードの開始
	dxCommon->BeginTextureUploadBatch();

	// 既存スプライト用テクスチャの読み込み
	texManager->LoadTexture("resources/uvChecker.png");
	texManager->LoadTexture("resources/monsterBall.png");
	texManager->LoadTexture("resources/checkerBoard.png");

	// Plane.obj のマテリアルで指定されているテクスチャがあれば登録する
	if (!modelDataPlaneObj.material.textureFilePath.empty())
	{
		texManager->LoadTexture(modelDataPlaneObj.material.textureFilePath);
	}

	// テクスチャアップロードの実行
	texManager->ExecuteUploadCommands();

	// TextureIndex を取得
	uint32_t uvCheckerTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/uvChecker.png");
	uint32_t monsterBallTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/monsterBall.png");
	uint32_t checkerBoardTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/checkerBoard.png");

	// Plane のテクスチャインデックス（存在しない場合は UINT_MAX）
	uint32_t planeTextureIndex = UINT32_MAX;
	if (!modelDataPlaneObj.material.textureFilePath.empty())
	{
		planeTextureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelDataPlaneObj.material.textureFilePath);
	}

	// 中間リソースをまとめて解放
	texManager->ClearIntermediateResources();

	// GPU 側の SRV ハンドルを準備（描画ループ外で用意）
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUPlane{};
	if (planeTextureIndex != UINT32_MAX)
	{
		textureSrvHandleGPUPlane = dxCommon->GetSRVGPUDescriptorHandle(planeTextureIndex);
	}
	else
	{
		// Plane にテクスチャが無ければデフォルトで uvChecker を使う
		textureSrvHandleGPUPlane = dxCommon->GetSRVGPUDescriptorHandle(uvCheckerTex);
	}

#pragma region 最初のシーンの初期化

	//スプライトのポインタ
	std::vector<Sprite*> sprites;

	//スプライトの初期化（5個作ってそれぞれ挫折を見られるように設定）
	const int kSpriteCount = 5;
	std::array<uint32_t, 3> textureIndices = { uvCheckerTex, monsterBallTex, checkerBoardTex };
	std::array<const char*, 3> textureNames = { "uvChecker.png", "monsterBall.png", "checkerBoard.png" };

	// 作成して初期設定
	for (int i = 0; i < kSpriteCount; ++i)
	{
		Sprite* s = new Sprite();
		// テクスチャは順繰りに割り当て
		s->Initialize(spriteCommon, textureIndices[i % textureIndices.size()]);

		// 初期位置を見やすく分散
		s->SetPosition(Vector2(50.0f + i * 140.0f, 120.0f + (i % 2) * 140.0f));
		// サイズを共通に（必要に応じて変えてください）
		s->SetSize(Vector2(128.0f, 128.0f));

		// アンカーポイントをバリエーション付与
		switch (i)
		{
		case 0: s->SetAnchorPoint(Vector2(0.0f, 0.0f)); break;   // 左上
		case 1: s->SetAnchorPoint(Vector2(0.5f, 0.5f)); break;   // 中央
		case 2: s->SetAnchorPoint(Vector2(1.0f, 1.0f)); break;   // 右下
		case 3: s->SetAnchorPoint(Vector2(0.0f, 0.5f)); break;   // 左中央
		case 4: s->SetAnchorPoint(Vector2(0.5f, 0.0f)); break;   // 上中央
		}

		// フリップを一部のスプライトで有効化して確認しやすくする
		s->SetFlipX(i == 2 || i == 4);
		s->SetFlipY(i == 1 || i == 3);

		// テクスチャの範囲指定（左上とサイズ）を一部に設定
		// ここではピクセル単位として指定している想定（Spriteの実装に合わせて必要なら正規化値に変更）
		if (i == 3)
		{
			s->SetTextureLeftTop(Vector2(0.0f, 0.0f));
			s->SetTextureSize(Vector2(64.0f, 64.0f));
		}
		else if (i == 4)
		{
			s->SetTextureLeftTop(Vector2(64.0f, 64.0f));
			s->SetTextureSize(Vector2(64.0f, 64.0f));
		}
		// その他はデフォルト（全体）を使用

		sprites.push_back(s);
	}

	// 選択用インデックス（ImGuiで操作するための状態）
	int selectedSpriteIndex = 0;

#pragma endregion

	Transform transformPlaneObj;
	transformPlaneObj.translate = { 0.0f,0.0f,0.0f };
	transformPlaneObj.rotation = { 0.0f,0.0f,0.0f };
	transformPlaneObj.scale = { 1.0f,1.0f,1.0f };

	Transform camera;
	camera.translate = { 0.0f,0.0f,-15.0f };
	camera.rotation = { 0.0f,0.0f,0.0f };
	camera.scale = { 1.0f,1.0f,1.0f };

	int32_t selectedModel = 0;

	bool isDisplaySprite = true;

	/*---メインループ---*/


	////ゲームループ
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


		/*--- Plane.objの更新処理 ---*/

		//Transformの更新
		Matrix4x4 worldMatrixPlaneObj = transformPlaneObj.GetWorldMatrix();
		Matrix4x4 cameraMatrixPlaneObj = camera.GetWorldMatrix();
		Matrix4x4 viewMatrixPlaneObj = Matrix4x4::Inverse(cameraMatrixPlaneObj);
		Matrix4x4 projectionMatrixPlaneObj = Matrix4x4::Perspective(0.45f, float(winApp->kClientWidth) / float(winApp->kClientHeight), 0.1f, 100.0f);
		//WVPMatrixの作成
		Matrix4x4 worldViewProjectionMatrixPlaneObj = Matrix4x4::Multiply(worldMatrixPlaneObj, Matrix4x4::Multiply(viewMatrixPlaneObj, projectionMatrixPlaneObj));
		transformationMatrixDataPlaneObj->WVP = worldViewProjectionMatrixPlaneObj;
		transformationMatrixDataPlaneObj->World = worldMatrixPlaneObj;

		/*--- Spriteの更新処理 ---*/
		for (uint32_t i = 0; i < 5; i++)
		{
			sprites[i]->Update();
		}


		//開発用UIの処理

		ImGui::Begin("window");


		//ImGui::Combo("ModelSelect", &selectedModel, modelNames, IM_ARRAYSIZE(modelNames));

		//ImGui::Checkbox("displaySprite", &isDisplaySprite);



		/*--- Plane.obj ---*/

		if (ImGui::CollapsingHeader("PlaneObj"))
		{
			//位置
			ImGui::DragFloat3("transform.translate", &transformPlaneObj.translate.x, 0.01f);

			// X軸の回転
			ImGui::SliderAngle("transform.rotate.X", &transformPlaneObj.rotation.x);

			// Y軸の回転
			ImGui::SliderAngle("transform.rotate.Y", &transformPlaneObj.rotation.y);

			// Z軸の回転
			ImGui::SliderAngle("transform.rotate.Z", &transformPlaneObj.rotation.z);

			//スケール
			ImGui::DragFloat3("transform.scale", &transformPlaneObj.scale.x, 0.01f);

			//カラー変更
			ImGui::ColorEdit4("Color", &(materialDataPlaneObj->color).x);

			//ライティングするかどうか
			ImGui::Checkbox("enableLighting", &materialDataPlaneObj->enableLighting);

			//Lightingの種類の設定
			/*ImGui::Combo("selectedLight", &materialDataPlaneObj->selectLightings, enableLightings, IM_ARRAYSIZE(enableLightings));*/
		}


		//カメラ設定のグループ
		if (ImGui::CollapsingHeader("camera"))
		{
			//位置
			ImGui::DragFloat3("camera.translate", &camera.translate.x, 0.01f);

			// X軸の回転
			ImGui::SliderAngle("camera.rotate.X", &camera.rotation.x);

			// Y軸の回転
			ImGui::SliderAngle("camera.rotate.Y", &camera.rotation.y);

			// Z軸の回転
			ImGui::SliderAngle("camera.rotate.Z", &camera.rotation.z);
		}


		//ライト設定のグループ
		if (ImGui::CollapsingHeader("Light"))
		{
			//向き
			ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.01f);

			//カラー
			ImGui::ColorEdit4("LightColor", &(directionalLightData->color).x);

			//輝度
			ImGui::SliderAngle("Lightrotate.Y", &directionalLightData->intensity);
		}

		// スプライト表示・編集のグループ
		if (isDisplaySprite)
		{
			if (ImGui::CollapsingHeader("Sprite"))
			{
				// スプライト一覧と選択
				if (ImGui::CollapsingHeader("Sprite Instances"))
				{
					// スプライト選択
					ImGui::SliderInt("Selected Sprite", &selectedSpriteIndex, 0, int(sprites.size()) - 1);

					// 簡易表示: 各スプライトのテクスチャ名・位置を表示（デバッグ用）
					for (int i = 0; i < (int)sprites.size(); ++i)
					{
						ImGui::Text("Sprite %d: pos=(%.1f,%.1f) size=(%.1f,%.1f) anchor=(%.2f,%.2f) flipX=%d flipY=%d",
							i,
							sprites[i]->GetPosition().x, sprites[i]->GetPosition().y,
							sprites[i]->GetSize().x, sprites[i]->GetSize().y,
							sprites[i]->GetAnchorPoint().x, sprites[i]->GetAnchorPoint().y,
							sprites[i]->IsFlipX() ? 1 : 0, sprites[i]->IsFlipY() ? 1 : 0);
					}
				}

				// 選択中スプライトの詳細編集
				Sprite* cur = sprites[selectedSpriteIndex];
				if (cur)
				{
					ImGui::Separator();
					ImGui::Text("Edit Sprite %d", selectedSpriteIndex);

					// 位置・回転・サイズ（既存のAPIを使う）
					Vector2 pos = cur->GetPosition();
					if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) cur->SetPosition(pos);

					float rot = cur->GetRotation();
					if (ImGui::SliderAngle("Rotation", &rot)) cur->SetRotation(rot);

					Vector2 size = cur->GetSize();
					if (ImGui::DragFloat2("Size", &size.x, 1.0f, 1.0f, 4096.0f)) cur->SetSize(size);

					// アンカーポイント
					Vector2 anchor = cur->GetAnchorPoint();
					if (ImGui::DragFloat2("Anchor (0..1)", &anchor.x, 0.01f, 0.0f, 1.0f))
					{
						cur->SetAnchorPoint(anchor);
					}

					// フリップ
					bool flipX = cur->IsFlipX();
					if (ImGui::Checkbox("Flip X", &flipX)) cur->SetFlipX(flipX);
					bool flipY = cur->IsFlipY();
					if (ImGui::Checkbox("Flip Y", &flipY)) cur->SetFlipY(flipY);

					// テクスチャ範囲（左上ピクセルと幅高さ）
					Vector2 texLeftTop = cur->GetTextureLeftTop();
					Vector2 texSize = cur->GetTextureSize();
					if (ImGui::DragFloat2("Texture LeftTop (px)", &texLeftTop.x, 1.0f, 0.0f, 4096.0f)) cur->SetTextureLeftTop(texLeftTop);
					if (ImGui::DragFloat2("Texture Size (px)", &texSize.x, 1.0f, 1.0f, 4096.0f)) cur->SetTextureSize(texSize);

					// テクスチャ切替
					static int selectedTex = 0;
					if (ImGui::Combo("Texture", &selectedTex, textureNames.data(), (int)textureNames.size()))
					{
						cur->SetTexture(textureIndices[selectedTex]);
					}

					// ボタンで次のテクスチャに切り替える
					if (ImGui::Button("Cycle Texture"))
					{
						int currentTex = selectedTex;
						currentTex = (currentTex + 1) % int(textureIndices.size());
						selectedTex = currentTex;
						cur->SetTexture(textureIndices[selectedTex]);
					}

					// 表示中の値（読み取り専用）
					ImGui::Text("Current texture leftTop=(%.1f,%.1f) size=(%.1f,%.1f)",
						cur->GetTextureLeftTop().x, cur->GetTextureLeftTop().y,
						cur->GetTextureSize().x, cur->GetTextureSize().y);
				}
			}
		}

		ImGui::End();


		/*-------------- ↓描画処理ここから↓ --------------*/

		//ImGuiの内部コマンドを生成
		ImGui::Render();

		dxCommon->PreDraw();

		//RootSignatureの設定
		dxCommon->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());

		//PSOを設定
		dxCommon->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());

		//形状を設定
		dxCommon->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		//VBVの設定
		dxCommon->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferViewPlaneObj);

		//IBVの設定
		dxCommon->GetCommandList()->IASetIndexBuffer(&indexBufferViewPlaneObj);

		//CBVの設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResourcePlaneObj->GetGPUVirtualAddress());

		//wvp用のCBufferの場所を設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(1, transformationMatrixResourcePlaneObj->GetGPUVirtualAddress());

		//SRVのDescriptorTableの先頭を設定
		dxCommon->GetCommandList()->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPUPlane);

		//平行光源用のCBufferの場所を設定
		dxCommon->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResouerce->GetGPUVirtualAddress());

		//描画！
		dxCommon->GetCommandList()->DrawIndexedInstanced(UINT(modelDataPlaneObj.indices.size()), 1, 0, 0, 0);


		//spriteCommon->SetCommonDrawSetting();
		//
		//if (isDisplaySprite)
		//{
		//	/*--- Sprite ---*/

		//	for (uint32_t i = 0; i < 5; i++)
		//	{
		//		sprites[i]->Draw();
		//	}
		//}

		//実際のCommandListのImGuiの描画コマンドを積む
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

		dxCommon->PostDraw();

	}

	//ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();


	//XAudio2の解放
	/*xAudio2.Reset();*/
	// テクスチャマネージャの解放
	TextureManager::GetInstance()->Finalize();

	//入力の解放
	delete input;

	//WindowsAPIの終了処理
	winApp->Finalize();

	//WindowsAPIの解放
	delete winApp;
	winApp = nullptr;



	//DirectX12の解放
	delete dxCommon;

	//スプライトの解放
	delete spriteCommon;

	//スプライトの解放
	for (uint32_t i = 0; i < 5; i++)
	{
		delete sprites[i];
	}

	////音声データ解放
	//SoundUnload(&soundData1);

	////解放処理
	//CloseHandle(fenceEvent);



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
	StringCchPrintfW(filePath, MAX_PATH,
		L"./Dumps/%04d-%02d-%02d-%02d-%02d.dmp",
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