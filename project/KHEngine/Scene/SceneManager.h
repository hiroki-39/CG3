#pragma once
#include "KHEngine/Core/Framework/BaseScene.h"

class SceneManager
{
public:

	~SceneManager();

	void Update();

	void Draw();

	// 次シーン予約
	void SetNextScene(BaseScene* NextScene) { nextScene_ = NextScene; }

private:
	// 次のシーン
	BaseScene* nextScene_ = nullptr;
	BaseScene* scene_ = nullptr;
};

