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
#include<wrl.h>
#include <functional>
#include<array>
#include<xaudio2.h>
#include <unordered_map>
#include <cassert>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib,"dxcompiler.lib")

#pragma comment(lib,"xaudio2.lib")

#include "KHEngine/Math/MathCommon.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Core/OS/WinApp.h"
#include <cstdint>
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Core/Graphics/D3DResourceLeakChecker.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Graphics/3d/Object/Object3d.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/3d/Model/ModelCommon.h"
#include "KHEngine/Graphics/3d/Model/Model.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <random>
#include "KHEngine/Graphics/3d/Particle/Particle.h"
#include "KHEngine/Graphics/3d/Particle/ParticleEmitter.h"
#include "KHEngine/Graphics/3d/Particle/ParticleSystem.h"
#include "KHEngine/Graphics/3d/Particle/ParticleRenderer.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"

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


enum class BlendMode
{
	Alpha = 0,
	Additive,
	Multiply,
	PreMultiplied,
	None,
	Count
};

// 以下のユーティリティ／プロトタイプは main 内で使用するものだけ保持
void Log(std::ostream& os, const std::string& message);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

void CreateWhiteTexture(DirectX::ScratchImage& outImage);

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

	//ModelCommonの初期化
	ModelManager::GetInstance()->Initialize(dxCommon);

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

	Object3dCommon* object3dCommon = nullptr;
	// 3Dオブジェクトの共通部分の初期化
	object3dCommon = new Object3dCommon();
	object3dCommon->Initialize(dxCommon);

	Camera* camera = new Camera();
	camera->SetTranslate({ 0.0f, 0.0f, -20.0f });
	camera->SetRotation({ 0.0f, 0.0f, 0.0f });
	object3dCommon->SetDefaultCamera(camera);

	SrvManager* srvManager = SrvManager::GetInstance();
	srvManager->Initialize(dxCommon);
	dxCommon->RegisterSrvManager(srvManager);

	TextureManager::GetInstance()->Initialize(dxCommon, srvManager);

#pragma endregion 

#pragma region パーティクル（Rendererへ移植）

	const uint32_t kNumMaxInstance = 100;

	ParticleRenderer particleRenderer;

	// アセット登録
	ParticleManager::GetInstance()->RegisterQuad("quad", "resources/circle.png");

	// インスタンシング配列などのポインタは renderer 初期化後に取得する（後で設定）
	ParticleForGPU* instancingData = nullptr;
	uint32_t instancingSrvIndex = UINT32_MAX;

	// 現在選択しているブレンドモード（UIで操作）
	int currentBlendModeIndex = static_cast<int>(BlendMode::Additive);


#pragma endregion 

	TextureManager* texManager = TextureManager::GetInstance();

	// テクスチャアップロードの開始
	dxCommon->BeginTextureUploadBatch();

	ModelManager::GetInstance()->LoadModel("plane.obj");

	// 既存スプライト用テクスチャの読み込み
	texManager->LoadTexture("resources/uvChecker.png");
	texManager->LoadTexture("resources/monsterBall.png");
	texManager->LoadTexture("resources/checkerBoard.png");
	texManager->LoadTexture("resources/circle.png");

	// テクスチャアップロードの実行
	texManager->ExecuteUploadCommands();

	// TextureIndex を取得
	uint32_t uvCheckerTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/uvChecker.png");
	uint32_t monsterBallTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/monsterBall.png");
	uint32_t checkerBoardTex = TextureManager::GetInstance()->GetTextureIndexByFilePath("resources/checkerBoard.png");

	// パーティクル用テクスチャのインデックス
	uint32_t particleSrvIndex = TextureManager::GetInstance()->GetSrvIndex("resources/circle.png");

	// ParticleRenderer を登録済みアセットからセットアップ
	ParticleManager::GetInstance()->SetupRendererFromAsset(particleRenderer, "quad", dxCommon, srvManager, kNumMaxInstance);
	instancingData = particleRenderer.GetInstancingData();
	instancingSrvIndex = particleRenderer.GetInstancingSrvIndex();

	// 中間リソースをまとめて解放
	texManager->ClearIntermediateResources();


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

	// ---------- 複数モデル用に変更 ----------
	const int kModelCount = 3;
	std::vector<Object3d*> modelInstances;
	modelInstances.reserve(kModelCount);
	for (int i = 0; i < kModelCount; ++i)
	{
		Object3d* obj = new Object3d();
		obj->Initialize(object3dCommon);
		obj->SetModel("plane.obj");
		obj->SetTranslate(Vector3(float(i) * 2.5f, 0.0f, 0.0f));
		obj->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
		obj->SetScale(Vector3(1.0f, 1.0f, 1.0f));
		modelInstances.push_back(obj);
	}

#pragma endregion

	//乱数生成器の初期化
	std::random_device seedGenerator;
	std::mt19937 randomEngine(seedGenerator());

	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	std::uniform_real_distribution<float> distColor(0.0f, 1.0f);
	std::uniform_real_distribution<float> distTime(1.0f, 3.0f);

	// ParticleSystem のセットアップ（Emitter 設定は ParticleSystem 経由で行う）
	ParticleSystem particleSystem;
	particleSystem.GetEmitter().GetEmitter().count = 3;
	particleSystem.GetEmitter().GetEmitter().frequency = 0.5f;
	particleSystem.GetEmitter().GetEmitter().frequencyTime = 0.0f;
	particleSystem.GetEmitter().GetEmitter().transform.translate = { 0.0f,-2.0f,0.0f };
	particleSystem.GetEmitter().GetEmitter().transform.rotation = { 0.0f,0.0f,0.0f };
	particleSystem.GetEmitter().GetEmitter().transform.scale = { 1.0f,1.0f,1.0f };

	// 現在のエフェクト選択（UIで変更可能）
	ParticleEffect currentEffect = ParticleEffect::Wind;
	particleSystem.SetEffect(currentEffect);

	// 初期パーティクルを生成（main の旧ループと同等数を生成）
	particleSystem.AddInitialParticles(randomEngine, kNumMaxInstance);

	const float kDeltaTime = 1.0f / 60.0f;

	//裏面回転行列
	Matrix4x4 baccktoFrontMatrix = Matrix4x4::RotateY(std::numbers::pi_v<float>);


	AccelerationField accelerationField;
	accelerationField.accleration = { -15.0f, 0.0f, 0.0f };
	accelerationField.area.min = { -1.0f, -1.0f, -1.0f };
	accelerationField.area.max = { 1.0f, 1.0f, 1.0f };

	particleSystem.SetAccelerationField(accelerationField);

	bool update = true;
	// ビルボード（カメラ目線）
	bool useBillboard = true;


	int32_t selectedModel = 0;

	bool isDisplaySprite = true;

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

		//ImGui_ImplDX12_NewFrame();
		//ImGui_ImplWin32_NewFrame();
		//ImGui::NewFrame();

		//入力の更新
		input->Update();


		/*-------------- ↓更新処理ここから↓ --------------*/

		camera->Update();

		/*--- 各モデルの更新処理 ---*/
		for (auto obj : modelInstances)
		{
			obj->Update();
		}

		/*--- Spriteの更新処理 ---*/
		for (uint32_t i = 0; i < 5; i++)
		{
			sprites[i]->Update();
		}


		// カメラ行列・ビュー・射影は Camera の getter を使う
		Matrix4x4 cameraMatrix = camera->GetWorldMatrix();
		Matrix4x4 viewMatrix = camera->GetViewMatrix();
		Matrix4x4 projectionMatrix = camera->GetProjectionMatrix();

		// ビルボード行列の計算（共通）
		Matrix4x4 billboardMatrix = Matrix4x4::Identity();

		if (useBillboard)
		{
			// カメラの回転成分のみを取り出して逆行列を使う（位置は除外）
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
			billboardMatrix = Matrix4x4::Identity();
		}

		// ParticleSystem を更新（Emitter の生成も内部で行う）
		if (update)
		{
			particleSystem.Update(kDeltaTime);
		}

		// Instancing バッファへ書き込み（GPU に転送するための CPU 側配列は Renderer が管理）
		uint32_t numInstance = particleSystem.FillInstancingBuffer(instancingData, kNumMaxInstance, viewMatrix, projectionMatrix, billboardMatrix, false);


		//開発用UIの処理


		/*-------------- ↓描画処理ここから↓ --------------*/

		//ImGuiの内部コマンドを生成
		/*ImGui::Render();*/

		dxCommon->PreDraw();

		object3dCommon->SetCommonDrawSetting();

		// 複数インスタンスの描画
		for (auto obj : modelInstances)
		{
			obj->Draw();
		}

		spriteCommon->SetCommonDrawSetting();

		if (isDisplaySprite)
		{
			/*--- Sprite ---*/

			for (uint32_t i = 0; i < 5; i++)
			{
				sprites[i]->Draw();
			}
		}

		// 描画は完全に ParticleRenderer に委譲
		particleRenderer.Draw(numInstance, particleSrvIndex, currentBlendModeIndex);

		//実際のCommandListのImGuiの描画コマンドを積む
		//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dxCommon->GetCommandList());

		dxCommon->PostDraw();

	}

	//ImGuiの終了処理
	//ImGui_ImplDX12_Shutdown();
	//ImGui_ImplWin32_Shutdown();
	//ImGui::DestroyContext();


	//XAudio2の解放
	/*xAudio2.Reset();*/

	// モデルマネージャーの解放
	ModelManager::GetInstance()->Finalize();

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

	// モデルインスタンスの解放
	for (auto obj : modelInstances)
	{
		delete obj;
	}
	modelInstances.clear();

	delete object3dCommon;

	srvManager->Finalize();

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
