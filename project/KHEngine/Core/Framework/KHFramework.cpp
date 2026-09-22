#include "KHFramework.h"
#include <combaseapi.h>
#include "KHEngine/Core/Graphics/D3DResourceLeakChecker.h"
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Debug/Editor/EditorSystem.cpp"
#include "KHEngine/Debug/Editor/EffectStudio.cpp"
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

	
	EditorSystem::GetInstance()->Initialize(dxCommon_.get());

	
	InitializeEngineSubsystems();

	// エンジンサブシステム（SrvManager等）の初期化完了後にエフェクトスタジオを初期化
	EffectStudio::GetInstance()->Initialize(dxCommon_.get(), srvManager_);
}

void KHFramework::FrameworkUpdate(float deltaTime)
{
	
	if (winApp_ && winApp_->ProcessMessage())
	{
		endRequest_ = true;
		return;
	}

	// エフェクトスタジオの更新
	EffectStudio::GetInstance()->Update(deltaTime);

	
	if (input_)
	{
		input_->Update();

		// F1キーでエディタモード（ImGui表示）をトグル
		if (input_->TriggerKey(DIK_F1))
		{
			auto services = EngineServices::GetInstance();
			if (services)
			{
				services->SetEditorMode(!services->GetEditorMode());
			}
		}
	}

	
	if (imguiManager_)
	{
		imguiManager_->Begin();

		
		
		
		EditorSystem::GetInstance()->Draw(postProcess_->GetResultSrvIndex());
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
	// エディターモード時、エフェクトスタジオのオフスクリーンレンダリングを実行
	if (EngineServices::GetInstance()->GetEditorMode())
	{
		EffectStudio::GetInstance()->Render();
	}

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
	particleMgr->RegisterRing("ring", "gradationLine.png", 32, 0.5f, 1.0f);
	particleMgr->RegisterCylinder("Cylinder", "resources/sprites/gradationLine.png");

	
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