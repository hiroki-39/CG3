#pragma once
#include <cstdint>
#include <memory>

#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Graphics/2d/SpriteCommon.h"
#include "KHEngine/Graphics/3d/Object/Object3dCommon.h"
#include "KHEngine/Graphics/3d/Model/ModelManager.h"
#include "KHEngine/Graphics/Resource/Texture/TextureManager.h"
#include "KHEngine/Graphics/Resource/Descriptor/SrvManager.h"
#include "KHEngine/Sound/Core/SoundManager.h"
#include "KHEngine/Scene/SceneFactory.h"
#include "KHEngine/Scene/AbstractSceneFactory.h"

class KHFramework
{
public:
    virtual ~KHFramework() = default;

    /// <summary>
    /// フレームワーク実行
    /// </summary>
    void Run();

protected:

    /// <summary>
    /// ゲーム初期化
    /// </summary>
    virtual void Initialize() {}

    /// <summary>
    /// ゲーム更新
    /// </summary>
    virtual void Update() {}

    /// <summary>
    /// ゲーム描画
    /// </summary>
    virtual void Draw() = 0;

    /// <summary>
    /// ゲーム終了処理
    /// </summary>
    virtual void Finalize() {}

protected:

    WinApp* winApp_ = nullptr;

    DirectXCommon* dxCommon_ = nullptr;
    
    Input* input_ = nullptr;
    
    ImGuiManager* imguiManager_ = nullptr;

    SpriteCommon* spriteCommon_ = nullptr;
    
    Object3dCommon* object3dCommon_ = nullptr;
    
    SrvManager* srvManager_ = nullptr;

    // フレーム関連設定（デフォルト 60fps）
    float kDeltaTime_ = 1.0f / 60.0f;

    // 終了要求
    bool endRequest_ = false;

private:

    // シーンファクトリー
	AbstractSceneFactory* sceneFactory_ = nullptr;

private:
    // ===== Framework 内部処理 =====
    void FrameworkInitialize();
    void FrameworkUpdate(float deltaTime);
    void FrameworkDrawBegin();
    void FrameworkDrawEnd();
    void FrameworkFinalize();

    // ------- ヘルパー -------

    // エンジンサブシステムの初期化／終了（Application ではなく Framework が責務）
    void InitializeEngineSubsystems();
    void FinalizeEngineSubsystems();

    // 1フレームの描画前後の共通処理（dxCommon の Pre/Post と ImGui の Begin/End をまとめる）
    void BeginFrameCommon();
    void EndFrameCommon();
};
