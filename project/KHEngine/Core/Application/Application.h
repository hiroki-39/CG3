#pragma once
#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Core/Framework/KHFramework.h"
#include "KHEngine/Scene/GamePlayScene.h"
#include "KHEngine/Scene/TitleScene.h"
#include "KHEngine/Core/Services/EngineServices.h"

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
    // ゲーム固有処理は GamePlayScene に移譲
   TitleScene* Scene_ = nullptr;
};