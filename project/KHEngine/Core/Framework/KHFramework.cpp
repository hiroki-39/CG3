#include "KHFramework.h"
#include <combaseapi.h>
#include "KHEngine/Core/Graphics/D3DResourceLeakChecker.h"
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Graphics/3d/Particle/ParticleManager.h"
#include "KHEngine/UI/UITextManager.h"
#ifdef ENABLE_EDITOR
#include "KHEngine/Debug/Editor/EditorSystem.cpp"
#include "KHEngine/Debug/Editor/EffectStudio.cpp"
#include "KHEngine/Debug/Editor/EnemyStudio.cpp"
#endif
#include "KHEngine/Core/Services/EngineServices.h"
#include <chrono>

void KHFramework::Run()
{
	
	FrameworkInitialize();

	
	Initialize();

	
	auto prevTime = std::chrono::high_resolution_clock::now();

	while (!endRequest_)
	{
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(currentTime - prevTime).count();
		prevTime = currentTime;

		
		if (deltaTime > 0.1f) deltaTime = 0.1f;

		EngineServices::GetInstance()->SetDeltaTime(deltaTime);

		FrameworkUpdate(deltaTime);
		Update();

		FrameworkDrawBegin();

		Draw();

		FrameworkDrawEnd();
	}

	
	Finalize();

	
	FrameworkFinalize();
}

void KHFramework::FrameworkInitialize()
{
	D3DResourceLeakChecker leakcheck;

	
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	
	Logger::Initialize();

	
	KHEngine::Core::Utility::Crash::CrashDump::Install();

	
	winApp_ = std::make_unique<WinApp>();
	winApp_->Initialize();

	
	dxCommon_ = std::make_unique<DirectXCommon>();
	dxCommon_->Initialize(winApp_.get());

	
	input_ = std::make_unique<Input>();
	input_->Initialize(winApp_.get());

	imguiManager_ = std::make_unique<ImGuiManager>();
	imguiManager_->Initialize(dxCommon_.get(), winApp_.get());

	// UIテキスト管理システムの初期化（設定JSONの読み込み）
	UITextManager::GetInstance()->Initialize();

#ifdef ENABLE_EDITOR
	EditorSystem::GetInstance()->Initialize(dxCommon_.get());
#endif

	
	InitializeEngineSubsystems();

#ifdef ENABLE_EDITOR
	// エンジンサブシステム（SrvManager等）の初期化完了後にエフェクトスタジオおよびエネミースタジオを初期化
	EffectStudio::GetInstance()->Initialize(dxCommon_.get(), srvManager_);
	EnemyStudio::GetInstance()->Initialize(dxCommon_.get(), srvManager_, object3dCommon_.get());
#endif
}

void KHFramework::FrameworkUpdate(float deltaTime)
{
	
	if (winApp_ && winApp_->ProcessMessage())
	{
		endRequest_ = true;
		return;
	}

#ifdef ENABLE_EDITOR
	// エフェクトスタジオ・エネミースタジオの更新
	EffectStudio::GetInstance()->Update(deltaTime);
	EnemyStudio::GetInstance()->Update(deltaTime);
#endif

	
	if (input_)
	{
		input_->Update();

#ifdef ENABLE_EDITOR
		// F1キーでエディタモード（ImGui表示）をトグル
		if (input_->TriggerKey(DIK_F1))
		{
			auto services = EngineServices::GetInstance();
			if (services)
			{
				services->SetEditorMode(!services->GetEditorMode());
			}
		}
#endif
	}

	// サウンドマネージャーの更新（再生終了したSEボイスの回収）
	SoundManager::GetInstance()->Update();

#ifdef USE_IMGUI
	if (imguiManager_)
	{
		imguiManager_->Begin();

#ifdef ENABLE_EDITOR
		if (EngineServices::GetInstance()->GetEditorMode())
		{
			EditorSystem::GetInstance()->Draw(postProcess_->GetResultSrvIndex());
		}
		else
#endif
		{
			// フルスクリーンプレイ時: ForegroundDrawListでUIテキストを描画
			UITextManager::GetInstance()->Draw(ImGui::GetForegroundDrawList(), ImVec2(0.0f, 0.0f), ImVec2(1280.0f, 720.0f));
		}
	}
#endif
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
#ifdef ENABLE_EDITOR
	// エディターモード時、エフェクトスタジオとエネミースタジオのオフスクリーンレンダリングを実行
	if (EngineServices::GetInstance()->GetEditorMode())
	{
		EffectStudio::GetInstance()->Render();
		EnemyStudio::GetInstance()->Render();
	}
#endif

	if (dxCommon_)
	{
		dxCommon_->PreDrawSwapchain();
	}

	
	if (postProcess_)
	{
		
		postProcess_->Draw(!EngineServices::GetInstance()->GetEditorMode());
	}

	
	DrawUI();
	if (postProcess_)
	{
		postProcess_->PostDraw(!EngineServices::GetInstance()->GetEditorMode());
	}

	
	if (imguiManager_)
	{
		imguiManager_->End();
		// エディターモードON、またはエディターモードOFF時のUIテキスト描画を反映
		imguiManager_->Draw();
	}

	if (dxCommon_)
	{
		dxCommon_->PostDraw();
	}
}

void KHFramework::FrameworkFinalize()
{
	
	FinalizeEngineSubsystems();

	
	if (imguiManager_)
	{
		imguiManager_->Finalize();
		imguiManager_.reset();
	}

	
	if (input_)
	{
		input_.reset();
	}

	
	if (dxCommon_)
	{
		dxCommon_.reset();
	}

	
	if (winApp_)
	{
		winApp_->Finalize();
		winApp_.reset();
	}

	
	KHEngine::Core::Utility::Crash::CrashDump::Uninstall();

	
	Logger::Shutdown();

	
	CoUninitialize();
}

/* ------- 繝倥Ν繝代・螳溯｣・------- */

void KHFramework::InitializeEngineSubsystems()
{
	
	srvManager_ = SrvManager::GetInstance();
	if (srvManager_ && dxCommon_)
	{
		srvManager_->Initialize(dxCommon_.get());
		
		dxCommon_->RegisterSrvManager(srvManager_);
	}

	
	TextureManager::GetInstance()->Initialize(dxCommon_.get(), srvManager_);

	
	if (dxCommon_)
	{
		dxCommon_->BeginTextureUploadBatch();
	}

	
	ModelManager::GetInstance()->Initialize(dxCommon_.get());

	
	spriteCommon_ = std::make_unique<SpriteCommon>();
	spriteCommon_->Initialize(dxCommon_.get());

	object3dCommon_ = std::make_unique<Object3dCommon>();
	object3dCommon_->Initialize(dxCommon_.get());

	
	// トランジション用ルール画像のロード
	TextureManager::GetInstance()->LoadTexture("resources/textures/rules/rule_horizontal.png");

	postProcess_ = std::make_unique<PostProcess>();
	postProcess_->Initialize(dxCommon_.get());
	EngineServices::GetInstance()->SetPostProcess(postProcess_.get());

	// パーティクル基本プリミティブ（Quad, Ring, Cylinder）の登録
	auto particleMgr = ParticleManager::GetInstance();
	particleMgr->RegisterQuad("quad", "circle2.png");
	particleMgr->RegisterRing("ring", "resources/sprites/effect/gradationLine.png", 32, 0.5f, 1.0f);
	particleMgr->RegisterCylinder("Cylinder", "resources/sprites/effect/gradationLine.png");

	
	TextureManager::GetInstance()->ExecuteUploadCommands();
	TextureManager::GetInstance()->ClearIntermediateResources();

	
	SoundManager::GetInstance()->Initialize();
}

void KHFramework::FinalizeEngineSubsystems()
{
	
	ModelManager::GetInstance()->Finalize();

	
	TextureManager::GetInstance()->Finalize();

	
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

	
	if (srvManager_)
	{
		srvManager_->Finalize();
		srvManager_ = nullptr;
	}

	
	SoundManager::GetInstance()->Finalize();
}

void KHFramework::BeginFrameCommon()
{
	
}

void KHFramework::EndFrameCommon()
{
	
}