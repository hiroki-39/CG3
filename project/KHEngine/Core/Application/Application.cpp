#define NOMINMAX
#include "Application.h"
#include <Windows.h>
#include <combaseapi.h>
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Core/Services/EngineServices.h"

// 初期化
void Application::Initialize()
{
    // 基底クラスの初期化（ウィンドウ・DirectX 等）
    KHFramework::Initialize();

    // フレームワークの共通オブジェクトを EngineServices に登録
    EngineServices* services = EngineServices::GetInstance();
    services->SetObject3dCommon(object3dCommon_);
    services->SetDirectXCommon(dxCommon_);
    services->SetSrvManager(srvManager_);
    services->SetSpriteCommon(spriteCommon_);
    services->SetInput(input_);
    services->SetImGuiManager(imguiManager_);

    // ゲーム固有の初期化はシーンへ
    gameplayScene_.Initialize();
}

// 終了処理
void Application::Finalize()
{
    // シーン側の終了処理
    gameplayScene_.Finalize();

    // 基底クラスの終了処理
    KHFramework::Finalize();
}

// 更新処理
void Application::Update()
{
    // 基底クラスの更新処理（フレーム開始・ImGui Begin 等を含む）
    KHFramework::Update();

    // シーン更新
    gameplayScene_.Update();

    // ImGui の描画受付終了（Framework が Begin している想定）
    if (imguiManager_)
    {
        imguiManager_->End();
    }
}

// 描画処理
void Application::Draw()
{
    // シーンがゲーム固有の描画を行う
    gameplayScene_.Draw();
}