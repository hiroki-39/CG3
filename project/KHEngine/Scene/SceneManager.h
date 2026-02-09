#pragma once
#include "KHEngine/Core/Framework/BaseScene.h"
#include "KHEngine/Scene/AbstractSceneFactory.h"

class SceneManager
{
public:

	~SceneManager();

	void Update();

	void Draw();

	/// <summary>
	/// 次のシーン予約
	/// </summary>
	/// <param name="sceneName">シーン名</param>
	void ChangeScene(const std::string& sceneName);

	// シーンファクトリーのSetter
	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

private:
	// 次のシーン
	BaseScene* nextScene_ = nullptr;
	BaseScene* scene_ = nullptr;

	AbstractSceneFactory* sceneFactory_ = nullptr;
};

