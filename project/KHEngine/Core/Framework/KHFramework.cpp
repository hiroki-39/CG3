#include "KHFramework.h"

#include <combaseapi.h>

#include "KHEngine/Core/Graphics/D3DResourceLeakChecker.h"
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Sound/Core/SoundManager.h"

// 各マネージャーはヘッダでインクルード済み（KHFramework.h）

void KHFramework::Run()
{
    // --- Framework 初期化 ---
    FrameworkInitialize();

    // --- ゲーム初期化 ---
    Initialize();

    // 固定 60fps（KHFramework::kDeltaTime_ を使っても良い）
    constexpr float kDeltaTime = 1.0f / 60.0f;

    // --- メインループ ---
    while (!endRequest_)
    {
        FrameworkUpdate(kDeltaTime);
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
    winApp_ = new WinApp();
    winApp_->Initialize();

    // --- DirectX ---
    dxCommon_ = new DirectXCommon();
    dxCommon_->Initialize(winApp_);

    // --- Input ---
    input_ = new Input();
    input_->Initialize(winApp_);

    // --- ImGui ---
    imguiManager_ = new ImGuiManager();
    imguiManager_->Initialize(dxCommon_, winApp_);

    // --- エンジンサブシステムの初期化 ---
    InitializeEngineSubsystems();
}

void KHFramework::FrameworkUpdate(float /*deltaTime*/)
{
    // Windows メッセージ処理
    if (winApp_->ProcessMessage())
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
    // ImGui 描画
    if (imguiManager_)
    {
        imguiManager_->Draw();
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
        delete imguiManager_;
        imguiManager_ = nullptr;
    }

    // --- Input ---
    if (input_)
    {
        delete input_;
        input_ = nullptr;
    }

    // --- DirectX ---
    if (dxCommon_)
    {
        delete dxCommon_;
        dxCommon_ = nullptr;
    }

    // --- Window ---
    if (winApp_)
    {
        winApp_->Finalize();
        delete winApp_;
        winApp_ = nullptr;
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
        srvManager_->Initialize(dxCommon_);
        // DirectXCommon に SRV 管理者を登録（必要なら）
        dxCommon_->RegisterSrvManager(srvManager_);
    }

    // Model 共通 / モデルマネージャ初期化
    ModelManager::GetInstance()->Initialize(dxCommon_);

    // SpriteCommon / Object3dCommon の初期化（描画設定や共通リソース）
    spriteCommon_ = new SpriteCommon();
    spriteCommon_->Initialize(dxCommon_);

    object3dCommon_ = new Object3dCommon();
    object3dCommon_->Initialize(dxCommon_);

    // TextureManager 初期化（SrvManager を渡す）
    TextureManager::GetInstance()->Initialize(dxCommon_, srvManager_);

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
        delete spriteCommon_;
        spriteCommon_ = nullptr;
    }

    if (object3dCommon_)
    {
        delete object3dCommon_;
        object3dCommon_ = nullptr;
    }

    // SRVマネージャのファイナライズ（シングルトンのFinalize）
    if (srvManager_)
    {
        srvManager_->Finalize();
        // srvManager_ はシングルトンなので delete しない
        srvManager_ = nullptr;
    }

    // サウンドの終了（必要ならここで）
    SoundManager::GetInstance()->Finalize();
}

void KHFramework::BeginFrameCommon()
{
    // 使う予定があれば補助処理をここへ（現在は FrameworkUpdate で Begin を行っている）
    // 将来的に BeginFrameCommon() を FrameworkUpdate() 内で呼ぶ設計に移せます。
}

void KHFramework::EndFrameCommon()
{
    // 使う予定があれば補助処理をここへ（現在は FrameworkDrawEnd で Draw/PostDraw を行っている）
    // 将来的に EndFrameCommon() を FrameworkDrawEnd() の内部で呼ぶ設計に移せます。
}
