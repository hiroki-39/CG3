#include "KHFramework.h"
#include <combaseapi.h>
#include "KHEngine/Core/Graphics/D3DResourceLeakChecker.h"
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Debug/Editor/EditorSystem.cpp"
#include "KHEngine/Core/Services/EngineServices.h"
#include <chrono>

void KHFramework::Run()
{
	// --- Framework 初期化 ---
	FrameworkInitialize();

	// --- ゲーム初期化 ---
	Initialize();

	// --- メインループ ---
	auto prevTime = std::chrono::high_resolution_clock::now();

	while (!endRequest_)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - prevTime).count();
		prevTime = currentTime;

		// 異常な遅延（ブレークポイントでの停止等）の対策として上限を設ける
		if (deltaTime > 0.1f) deltaTime = 0.1f;

		EngineServices::GetInstance()->SetDeltaTime(deltaTime);

		FrameworkUpdate(deltaTime);
		Update();

		FrameworkDrawBegin();

		Draw();

		FrameworkDrawEnd();
	}

	// --- ゲーム終了 ---
	Finalize();

	// --- Framework 終了 ---
	FrameworkFinalize();
}

void KHFramework::FrameworkInitialize()
{
	D3DResourceLeakChecker leakcheck;

	// COM 初期化
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// ログ
	Logger::Initialize();

	// クラッシュダンプ
	KHEngine::Core::Utility::Crash::CrashDump::Install();

	// --- Window ---
	winApp_ = std::make_unique<WinApp>();
	winApp_->Initialize();

	// --- DirectX ---
	dxCommon_ = std::make_unique<DirectXCommon>();
	dxCommon_->Initialize(winApp_.get());

	// --- Input ---
	input_ = std::make_unique<Input>();
	input_->Initialize(winApp_.get());

	imguiManager_ = std::make_unique<ImGuiManager>();
	imguiManager_->Initialize(dxCommon_.get(), winApp_.get());

	// --- Editor ---
	EditorSystem::GetInstance()->Initialize(dxCommon_.get());

	// --- エンジンサブシステムの初期化 ---
	InitializeEngineSubsystems();
}

void KHFramework::FrameworkUpdate(float /*deltaTime*/)
{
	// Windows メッセージ処理
	if (winApp_ && winApp_->ProcessMessage())
	{
		endRequest_ = true;
		return;
	}

	// 入力更新
	if (input_)
	{
		input_->Update();
	}

	// ImGui 開始（各フレームのUI受付開始）
	if (imguiManager_)
	{
		imguiManager_->Begin();

		if (EngineServices::GetInstance()->GetEditorMode())
		{
			// エディタモード：最初にドッキング空間を作り、その中にビューポートを表示
			// （他のImGui::Beginより前に呼ぶ必要があるためここで行う）
			EditorSystem::GetInstance()->Draw(postProcess_->GetResultSrvIndex());
		}

		// ポストプロセスのエディタウィンドウを表示
		if (postProcess_)
		{
			postProcess_->DrawImGui();
		}
	}
}

void KHFramework::FrameworkDrawBegin()
{
	if (dxCommon_)
	{
		dxCommon_->PreDraw();
	}
}

void KHFramework::FrameworkDrawEnd()
{
	// Swapchainへの描画準備
	if (dxCommon_)
	{
		dxCommon_->PreDrawSwapchain();
	}

	// PostProcessを描画
	if (postProcess_)
	{
		// エディタモードなら専用のテクスチャへ、ゲームモードなら直接Swapchainへ出力
		postProcess_->Draw(!EngineServices::GetInstance()->GetEditorMode());
	}

	// ImGui 描画準備と描画
	if (imguiManager_)
	{
		imguiManager_->End();
		if (EngineServices::GetInstance()->GetEditorMode())
		{
			imguiManager_->Draw();
		}
	}

	if (dxCommon_)
	{
		dxCommon_->PostDraw();
	}
}

void KHFramework::FrameworkFinalize()
{
	// エンジンサブシステムの終了
	FinalizeEngineSubsystems();

	// --- ImGui ---
	if (imguiManager_)
	{
		imguiManager_->Finalize();
		imguiManager_.reset();
	}

	// --- Input ---
	if (input_)
	{
		input_.reset();
	}

	// --- DirectX ---
	if (dxCommon_)
	{
		dxCommon_.reset();
	}

	// --- Window ---
	if (winApp_)
	{
		winApp_->Finalize();
		winApp_.reset();
	}

	// クラッシュダンプ解除
	KHEngine::Core::Utility::Crash::CrashDump::Uninstall();

	// ログ終了
	Logger::Shutdown();

	// COM 終了
	CoUninitialize();
}

/* ------- ヘルパー実装 ------- */

void KHFramework::InitializeEngineSubsystems()
{
	// SRV マネージャーの初期化（DirectXCommon が必要）
	srvManager_ = SrvManager::GetInstance();
	if (srvManager_ && dxCommon_)
	{
		srvManager_->Initialize(dxCommon_.get());
		// DirectXCommon に SRV 管理者を登録（必要なら）
		dxCommon_->RegisterSrvManager(srvManager_);
	}

	// TextureManager の初期化
	TextureManager::GetInstance()->Initialize(dxCommon_.get(), srvManager_);

	// バッチ開始（ここから ExecuteUploadCommands までの間にロードされたテクスチャが GPU に送られる）
	if (dxCommon_)
	{
		dxCommon_->BeginTextureUploadBatch();
	}

	// Model 共通 / モデルマネージャ初期化
	ModelManager::GetInstance()->Initialize(dxCommon_.get());

	// SpriteCommon / Object3dCommon の初期化
	spriteCommon_ = std::make_unique<SpriteCommon>();
	spriteCommon_->Initialize(dxCommon_.get());

	object3dCommon_ = std::make_unique<Object3dCommon>();
	object3dCommon_->Initialize(dxCommon_.get());

	// PostProcess 初期化
	postProcess_ = std::make_unique<PostProcess>();
	postProcess_->Initialize(dxCommon_.get());
	EngineServices::GetInstance()->SetPostProcess(postProcess_.get());

	// バッチアップロードを実行して中間リソースを解放
	TextureManager::GetInstance()->ExecuteUploadCommands();
	TextureManager::GetInstance()->ClearIntermediateResources();

	// SoundManager 初期化
	SoundManager::GetInstance()->Initialize();
}

void KHFramework::FinalizeEngineSubsystems()
{
	// モデルマネージャの解放（GPUリソースを解放）
	ModelManager::GetInstance()->Finalize();

	// テクスチャマネージャの解放
	TextureManager::GetInstance()->Finalize();

	// SpriteCommon / Object3dCommon の解放（確保順の逆で安全に）
	if (spriteCommon_)
	{
		spriteCommon_.reset();
	}

	if (object3dCommon_)
	{
		object3dCommon_.reset();
	}

	if (postProcess_)
	{
		postProcess_.reset();
	}

	// SRVマネージャのファイナライズ（シングルトンのFinalize）
	if (srvManager_)
	{
		srvManager_->Finalize();
		srvManager_ = nullptr;
	}

	// サウンドの終了（必要ならここで）
	SoundManager::GetInstance()->Finalize();
}

void KHFramework::BeginFrameCommon()
{
	// 使う予定があれば補助処理をここへ（現在は FrameworkUpdate で Begin を行っている）
}

void KHFramework::EndFrameCommon()
{
	// 使う予定があれば補助処理をここへ（現在は FrameworkDrawEnd で Draw/PostDraw を行っている）
}