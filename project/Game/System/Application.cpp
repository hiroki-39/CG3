#define NOMINMAX
#include "Application.h"
#include <Windows.h>
#include <combaseapi.h>
#include "KHEngine/Core/Utility/Log/Logger.h"
#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Core/Services/EngineServices.h"
#include "Game/Scene/GameSceneFactory.h"
#include <memory>

// 蛻晄悄蛹・
void Application::Initialize()
{
    // 蝓ｺ蠎輔け繝ｩ繧ｹ縺ｮ蛻晄悄蛹・
    KHFramework::Initialize();

    // 繝輔Ξ繝ｼ繝繝ｯ繝ｼ繧ｯ縺ｮ蜈ｱ騾壹が繝悶ず繧ｧ繧ｯ繝医ｒ EngineServices 縺ｫ逋ｻ骭ｲ
    EngineServices* services = EngineServices::GetInstance();
    services->SetObject3dCommon(object3dCommon_.get());
    services->SetDirectXCommon(dxCommon_.get());
    services->SetSrvManager(srvManager_);
    services->SetSpriteCommon(spriteCommon_.get());
    services->SetInput(input_.get());
    services->SetImGuiManager(imguiManager_.get());

    // 繧ｷ繝ｼ繝ｳ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ縺ｨ繧ｷ繝ｼ繝ｳ繝輔ぃ繧ｯ繝医Μ繝ｼ縺ｮ逕滓・縺ｨ蛻晄悄險ｭ螳・
    sceneFactory_ = std::make_unique<GameSceneFactory>();
    services->SetSceneFactory(sceneFactory_.get());

    // SceneManager 繧堤函謌舌＠縺ｦ繝輔ぃ繧ｯ繝医Μ繝ｼ繧定ｨｭ螳・
    sceneManager_ = std::make_unique<SceneManager>();
    sceneManager_->SetSceneFactory(sceneFactory_.get());

    // 襍ｷ蜍墓凾縺ｯ繧ｿ繧､繝医Ν繧ｷ繝ｼ繝ｳ繧剃ｺ育ｴ・＠縺ｦ縺九ｉ Update 繧貞他縺ｶ
    sceneManager_->ChangeScene("TITLE");

    // 莠育ｴ・＆繧後◆蛻晄悄繧ｷ繝ｼ繝ｳ繧貞叉譎ょ・譖ｿ繝ｻ蛻晄悄蛹悶☆繧・
    if (sceneManager_)
    {
        sceneManager_->Update();
    }
}

// 邨ゆｺ・・逅・
void Application::Finalize()
{
    // 繧ｷ繝ｼ繝ｳ繝槭ロ繝ｼ繧ｸ繝ｼ縺ｮ遐ｴ譽・ｼ亥・驛ｨ縺ｧ迴ｾ蝨ｨ繧ｷ繝ｼ繝ｳ縺ｮ Finalize/Delete 繧定｡後≧・・
    if (sceneManager_)
    {
        sceneManager_.reset();
    }

    // 繧ｷ繝ｼ繝ｳ繝輔ぃ繧ｯ繝医Μ繝ｼ縺ｮ遐ｴ譽・
    if (sceneFactory_)
    {
        sceneFactory_.reset();
    }

    // 蝓ｺ蠎輔け繝ｩ繧ｹ縺ｮ邨ゆｺ・・逅・
    KHFramework::Finalize();
}

// 譖ｴ譁ｰ蜃ｦ逅・
void Application::Update()
{
    // 蝓ｺ蠎輔け繝ｩ繧ｹ縺ｮ譖ｴ譁ｰ蜃ｦ逅・ｼ医ヵ繝ｬ繝ｼ繝髢句ｧ九・ImGui Begin 遲峨ｒ蜷ｫ繧・・
    KHFramework::Update();

    // 繧ｷ繝ｼ繝ｳ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ譖ｴ譁ｰ・医す繝ｼ繝ｳ縺ｮ Update 繧貞他縺ｶ・・
    if (sceneManager_)
    {
        sceneManager_->Update();
    }
}

// 謠冗判蜃ｦ逅・
void Application::Draw()
{
    // 繧ｷ繝ｼ繝ｳ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ縺ｫ謠冗判繧貞ｧ碑ｭｲ
    if (sceneManager_)
    {
        sceneManager_->Draw();
    }
}