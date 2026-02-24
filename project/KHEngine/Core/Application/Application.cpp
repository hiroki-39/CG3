#define NOMINMAX
#include "Application.h"
#include <Windows.h>
#include <combaseapi.h>
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Scene/SceneFactory.h"
#include <memory>

// 初期化
void Application::Initialize()
{
    // 基底クラスの初期化
    KHFramework::Initialize();

    // フレームワークの共通オブジェクトを EngineServices に登録
    EngineServices* services = EngineServices::GetInstance();
    services->SetObject3dCommon(object3dCommon_.get());
    services->SetDirectXCommon(dxCommon_.get());
    services->SetSrvManager(srvManager_);
    services->SetSpriteCommon(spriteCommon_.get());
    services->SetInput(input_.get());
    services->SetImGuiManager(imguiManager_.get());

    // シーンマネージャーとシーンファクトリーの生成と初期設定
    sceneFactory_ = std::make_unique<SceneFactory>();
    services->SetSceneFactory(sceneFactory_.get());

    // SceneManager を生成してファクトリーを設定
    sceneManager_ = std::make_unique<SceneManager>();
    sceneManager_->SetSceneFactory(sceneFactory_.get());

    // 起動時はタイトルシーンを予約してから Update を呼ぶ
    sceneManager_->ChangeScene("TITLE");

    // 予約された初期シーンを即時切替・初期化する
    if (sceneManager_)
    {
        sceneManager_->Update();
    }
}

// 終了処理
void Application::Finalize()
{
    // シーンマネージーの破棄（内部で現在シーンの Finalize/Delete を行う）
    if (sceneManager_)
    {
        sceneManager_.reset();
    }

    // シーンファクトリーの破棄
    if (sceneFactory_)
    {
        sceneFactory_.reset();
    }

    // 基底クラスの終了処理
    KHFramework::Finalize();
}

// 更新処理
void Application::Update()
{
    // 基底クラスの更新処理（フレーム開始・ImGui Begin 等を含む）
    KHFramework::Update();

    // シーンマネージャー更新（シーンの Update を呼ぶ）
    if (sceneManager_)
    {
        sceneManager_->Update();
    }

    // ImGui の描画受付終了（Framework が Begin している想定）
    if (imguiManager_)
    {
        imguiManager_->End();
    }
}

// 描画処理
void Application::Draw()
{
    // シーンマネージャーに描画を委譲
    if (sceneManager_)
    {
        sceneManager_->Draw();
    }
}