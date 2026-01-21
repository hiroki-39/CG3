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
#include <fstream>
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
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Sound/Core/Sound.h"

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

	// ImGuiの初期化
	ImGuiManager* imguiManager = new ImGuiManager();
	imguiManager->Initialize(dxCommon, winApp);

#pragma endregion 

#pragma region パーティクル

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

	// スプライト
	std::vector<Sprite*> sprites;
	{
		Sprite* s = new Sprite();
		// テクスチャはuvChecker
		s->Initialize(spriteCommon, uvCheckerTex);
		// 初期位置を(100,100)
		s->SetPosition(Vector2(100.0f, 100.0f));
		// サイズ
		s->SetSize(Vector2(128.0f, 128.0f));
		// アンカーポイントを中央に
		s->SetAnchorPoint(Vector2(0.5f, 0.5f));
		// 色
		s->SetColor(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		sprites.push_back(s);
	}

	// モデル
	std::vector<Object3d*> modelInstances;
	{
		Object3d* obj = new Object3d();
		obj->Initialize(object3dCommon);
		obj->SetModel("monsterBall.obj");
		obj->SetTranslate(Vector3(0.0f, 0.0f, 0.0f));
		obj->SetRotation(Vector3(0.0f, 0.0f, 0.0f));
		obj->SetScale(Vector3(1.0f, 1.0f, 1.0f));
		modelInstances.push_back(obj);
	}

	// サウンドマネージャーの初期化
	SoundManager::GetInstance()->Initialize();

	// mp3を読み込む
	static SoundManager::SoundData Data = SoundManager::GetInstance()->SoundLoadFile("resources/bgm.mp3");

	// 再生用オブジェクト
	Sound sound;


#pragma endregion

	// 乱数生成器の初期化
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

	// 裏面回転行列
	Matrix4x4 baccktoFrontMatrix = Matrix4x4::RotateY(std::numbers::pi_v<float>);


	AccelerationField accelerationField;
	accelerationField.accleration = { -15.0f, 0.0f, 0.0f };
	accelerationField.area.min = { -1.0f, -1.0f, -1.0f };
	accelerationField.area.max = { 1.0f, 1.0f, 1.0f };

	particleSystem.SetAccelerationField(accelerationField);

	bool update = true;
	// ビルボード（カメラ目線）
	bool useBillboard = true;

	bool isDisplaySprite = true;

	/*---メインループ---*/

	// ゲームループ
	while (true)
	{
		// Windowsのメッセージ処理
		if (winApp->ProcessMessage())
		{
			// ゲームループを抜ける
			break;
		}

		/*-------------- ↓更新処理ここから↓ --------------*/

		// 入力の更新
		input->Update();

		// カメラの更新
		camera->Update();

		/*--- 各モデルの更新処理 ---*/
		for (auto model : modelInstances)
		{
			model->Update();
		}

		/*--- Spriteの更新処理 ---*/
		for (auto sprite : sprites)
		{
			sprite->Update();
		}

		if (input->TriggerKey(DIK_SPACE))
		{
			// サウンドの再生
			sound.SoundPlayWave(SoundManager::GetInstance()->GetXAudio2(), Data);

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


		// 開発用UIの処理
		imguiManager->Begin();


#ifdef USE_IMGUI

		ImGui::Begin("Scene Controls");

		// Sprite
		if (ImGui::CollapsingHeader("Sprite"))
		{
			if (!sprites.empty())
			{
				Sprite* s = sprites[0];

				bool display = isDisplaySprite;
				ImGui::Checkbox("Display Sprite", &display);
				isDisplaySprite = display;

				Vector2 pos = s->GetPosition();
				float posArr[2] = { pos.x, pos.y };
				if (ImGui::DragFloat2("Position", posArr, 1.0f))
				{
					s->SetPosition(Vector2(posArr[0], posArr[1]));
				}

				Vector2 size = s->GetSize();
				float sizeArr[2] = { size.x, size.y };
				if (ImGui::DragFloat2("Size", sizeArr, 1.0f, 1.0f, 4096.0f))
				{
					s->SetSize(Vector2(sizeArr[0], sizeArr[1]));
				}

				float rotation = s->GetRotation();
				if (ImGui::DragFloat("Rotation", &rotation, 0.5f))
				{
					s->SetRotation(rotation);
				}

				Vector4 col = s->GetColor();
				float colArr[4] = { col.x, col.y, col.z, col.w };
				if (ImGui::ColorEdit4("Color", colArr))
				{
					s->SetColor(Vector4(colArr[0], colArr[1], colArr[2], colArr[3]));
				}
			}
		}

		// Model
		if (ImGui::CollapsingHeader("Model"))
		{
			if (!modelInstances.empty())
			{
				Object3d* obj = modelInstances[0];
				// Translate / Rotation / Scale
				Vector3 t = obj->GetTranslate();
				float tArr[3] = { t.x, t.y, t.z };
				if (ImGui::DragFloat3("Translate", tArr, 0.05f))
				{
					obj->SetTranslate(Vector3(tArr[0], tArr[1], tArr[2]));
				}

				Vector3 r = obj->GetRotation();
				float rArr[3] = { r.x, r.y, r.z };
				if (ImGui::DragFloat3("Rotation", rArr, 0.5f))
				{
					obj->SetRotation(Vector3(rArr[0], rArr[1], rArr[2]));
				}

				Vector3 s = obj->GetScale();
				float sArr[3] = { s.x, s.y, s.z };
				if (ImGui::DragFloat3("Scale", sArr, 0.01f, 0.001f, 100.0f))
				{
					obj->SetScale(Vector3(sArr[0], sArr[1], sArr[2]));
				}

			}
		}

		// Dedicated Light パネル
		if (ImGui::CollapsingHeader("Light"))
		{
			if (!modelInstances.empty())
			{
				Object3d* obj = modelInstances[0];

				// Color
				Vector4 lc = obj->GetDirectionalLightColor();
				float lcArr[4] = { lc.x, lc.y, lc.z, lc.w };
				if (ImGui::ColorEdit4("Directional Color", lcArr))
				{
					obj->SetDirectionalLightColor(Vector4(lcArr[0], lcArr[1], lcArr[2], lcArr[3]));
				}

				// Direction (normalized on update inside Object3d)
				Vector3 ld = obj->GetDirectionalLightDirection();
				float ldArr[3] = { ld.x, ld.y, ld.z };
				if (ImGui::DragFloat3("Directional Direction", ldArr, 0.05f, -10.0f, 10.0f))
				{
					obj->SetDirectionalLightDirection(Vector3(ldArr[0], ldArr[1], ldArr[2]));
				}

				// Intensity
				float lint = obj->GetDirectionalLightIntensity();
				if (ImGui::DragFloat("Directional Intensity", &lint, 0.01f, 0.0f, 100.0f))
				{
					obj->SetDirectionalLightIntensity(lint);
				}
			}
			else
			{
				ImGui::Text("No model instances available to control light.");
			}
		}

		// Camera
		if (ImGui::CollapsingHeader("Camera"))
		{
			// Camera position / rotation via direct reference getters
			Vector3& camPosRef = camera->GetTranslate();
			float camPosArr[3] = { camPosRef.x, camPosRef.y, camPosRef.z };
			if (ImGui::DragFloat3("Cam Translate", camPosArr, 0.1f))
			{
				camera->SetTranslate(Vector3(camPosArr[0], camPosArr[1], camPosArr[2]));
			}

			Vector3& camRotRef = camera->GetRotation();
			float camRotArr[3] = { camRotRef.x, camRotRef.y, camRotRef.z };
			if (ImGui::DragFloat3("Cam Rotation", camRotArr, 0.5f))
			{
				camera->SetRotation(Vector3(camRotArr[0], camRotArr[1], camRotArr[2]));
			}

			// FOV / aspect / near / far (getters added)
			float fov = camera->GetFovY();
			if (ImGui::DragFloat("FOV Y", &fov, 0.01f, 0.01f, 3.14f))
			{
				camera->SetFovY(fov);
			}

			float aspect = camera->GetAspectRatio();
			if (ImGui::DragFloat("Aspect", &aspect, 0.01f, 0.1f, 10.0f))
			{
				camera->SetAspectRatio(aspect);
			}

			float nearC = camera->GetNearClip();
			if (ImGui::DragFloat("Near Clip", &nearC, 0.001f, 0.001f, 100.0f))
			{
				camera->SetNearClip(nearC);
			}

			float farC = camera->GetFarClip();
			if (ImGui::DragFloat("Far Clip", &farC, 0.1f, 1.0f, 1000.0f))
			{
				camera->SetFarClip(farC);
			}
		}

		ImGui::End();

#endif // USE_IMGUI

		// ImGuiの内部コマンドを生成
		imguiManager->End();

		/*-------------- ↓描画処理ここから↓ --------------*/

		dxCommon->PreDraw();

		object3dCommon->SetCommonDrawSetting();

		// 複数インスタンスの描画
		for (auto model : modelInstances)
		{
			model->Draw();
		}

		spriteCommon->SetCommonDrawSetting();

		if (isDisplaySprite)
		{
			for (auto sprite : sprites)
			{
				sprite->Draw();
			}
		}

		// 描画は完全に ParticleRenderer に委譲
		particleRenderer.Draw(numInstance, particleSrvIndex, currentBlendModeIndex);

		// 実際のCommandListのImGuiの描画コマンドを積む
		imguiManager->Draw();

		dxCommon->PostDraw();

	}

	// ImGuiの終了処理
	imguiManager->Finalize();



	// モデルマネージャーの解放
	ModelManager::GetInstance()->Finalize();

	// テクスチャマネージャの解放
	TextureManager::GetInstance()->Finalize();

	// 入力の解放
	delete input;

	delete imguiManager;

	// WindowsAPIの終了処理
	winApp->Finalize();

	// WindowsAPIの解放
	delete winApp;
	winApp = nullptr;

	// DirectX12の解放
	delete dxCommon;

	// スプライト共通部分の解放
	delete spriteCommon;

	// スプライトインスタンスの解放
	for (auto s : sprites)
	{
		delete s;
	}

	// モデルインスタンスの解放
	for (auto obj : modelInstances)
	{
		delete obj;
	}

	modelInstances.clear();

	// 3Dオブジェクト共通部分の解放
	delete object3dCommon;

	sound.Stop();
	SoundManager::GetInstance()->SoundUnload(&Data);
	SoundManager::GetInstance()->Finalize();

	srvManager->Finalize();

	return 0;
}

//---ここから下は関数の実装---//

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
