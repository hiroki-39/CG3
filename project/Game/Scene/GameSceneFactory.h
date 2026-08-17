#pragma once
#include "KHEngine/Scene/AbstractSceneFactory.h"
#include "TitleScene.h"
#include "GamePlayScene.h"
#include "GameOverScene.h"
#include "GameClearScene.h"

class GameSceneFactory : public AbstractSceneFactory
{
public:
    std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override
    {
        if (sceneName == "TITLE") return std::make_unique<TitleScene>();
        if (sceneName == "GAMEPLAY") return std::make_unique<GamePlayScene>();
        if (sceneName == "GAMEOVER") return std::make_unique<GameOverScene>();
        if (sceneName == "GAMECLEAR") return std::make_unique<GameClearScene>();
        return nullptr;
    }
};