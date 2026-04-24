#pragma once
#include "KHEngine/Scene/AbstractSceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"

class GameSceneFactory : public AbstractSceneFactory
{
public:
    std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override
    {
        if (sceneName == "TITLE") return std::make_unique<TitleScene>();
        if (sceneName == "GAMEPLAY") return std::make_unique<GamePlayScene>();
        return nullptr;
    }
};