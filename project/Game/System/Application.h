#pragma once
#include "KHEngine/Core/OS/WinApp.h"
#include "KHEngine/Core/Graphics/DirectXCommon.h"
#include "KHEngine/Input/Input.h"
#include "KHEngine/Debug/Imgui/ImGuiManager.h"
#include "KHEngine/Core/Framework/KHFramework.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "KHEngine/Scene/SceneManager.h"
#include "KHEngine/Scene/AbstractSceneFactory.h"
#include "Game/Scene/GameSceneFactory.h"
#include <memory>

class Application : public KHFramework
{

public:

    /// <summary>
    /// 蛻晄悄蛹・
    /// </summary>
    void Initialize() override;

    /// <summary>
    /// 邨ゆｺ・・逅・
    /// </summary>
    void Finalize() override;

    /// <summary>
    /// 豈弱ヵ繝ｬ繝ｼ繝譖ｴ譁ｰ蜃ｦ逅・
    /// </summary>
    void Update() override;

    /// <summary>
    /// 謠冗判蜃ｦ逅・
    /// </summary>
    void Draw() override;

private:
    // 繧ｷ繝ｼ繝ｳ邂｡逅・勣・域園譛峨ｒ譏守､ｺ・・
    std::unique_ptr<SceneManager> sceneManager_ = nullptr;

    // 繧ｷ繝ｼ繝ｳ繝輔ぃ繧ｯ繝医Μ繝ｼ・域歓雎｡蝙九〒菫晄戟縲∵園譛峨ｒ譏守､ｺ・・
    std::unique_ptr<AbstractSceneFactory> sceneFactory_ = nullptr;
};