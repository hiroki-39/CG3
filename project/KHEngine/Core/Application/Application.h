#pragma once
#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Core/Framework/KHFramework.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Scene/SceneManager.h"

class Application : public KHFramework
{

public:

    /// <summary>
    /// 初期化
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize() override;

    /// <summary>
    /// 毎フレーム更新処理
    /// </summary>
    void Update() override;

    /// <summary>
    /// 描画処理
    /// </summary>
    void Draw() override;

private:
    // シーン管理器
    SceneManager* sceneManager_ = nullptr;
};