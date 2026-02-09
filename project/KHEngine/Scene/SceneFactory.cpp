#include "SceneFactory.h"
#include "KHEngine/Scene/TitleScene.h"
#include "KHEngine/Scene/GamePlayScene.h"

BaseScene* SceneFactory::CreateScene(const std::string& sceneName)
{
	// 次のシーンを生成
	BaseScene* newScene = nullptr;

	if (sceneName == "TITLE")
	{
		newScene = new TitleScene();
	}
	else if (sceneName == "GAMEPLAY")
	{
		newScene = new GamePlayScene();
	}

	return newScene;
}
