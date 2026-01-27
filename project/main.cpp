#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <string>
#include <format>
#include<dbghelp.h>
#include <strsafe.h>
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <numbers>
#include <iomanip>
#include <wrl.h>
#include <functional>
#include <array>
#include <xaudio2.h>
#include <fstream>
#include <unordered_map>
#include <cassert>

#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib,"dxcompiler.lib")

#pragma comment(lib,"xaudio2.lib")

#include "KHEngine/Math/MathCommon.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <cstdint>
#include "KHEngine/Graphics/3d/Model/ModelCommon.h"
#include "KHEngine/Graphics/3d/Model/Model.h"
#include "KHEngine/Graphics/3d/Camera/Camera.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include <random>
#include "KHEngine/Graphics/3d/Particle/ParticleEmitter.h"
#include "KHEngine/Graphics/3d/Particle/ParticleSystem.h"
#include "KHEngine/Graphics/Billboard/Billboard.h"

#include "KHEngine/Application/Application.h"

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

	// アプリケーションの初期化
	Application* app;

	// アプリケーションの初期化
	app->Initialize();

	// ゲームループ
	while (true)
	{
	
		app->Update();

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

	// アンインストール
	KHEngine::Core::Utility::Crash::CrashDump::Uninstall();
	// シャットダウン
	Logger::Shutdown();

	return 0;
}