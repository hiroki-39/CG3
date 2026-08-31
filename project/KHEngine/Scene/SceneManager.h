#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include "KHEngine/Core/Framework/BaseScene.h"
#include "KHEngine/Scene/AbstractSceneFactory.h"

class SceneManager
{
public:

	~SceneManager();

	void Update();

	void Draw();
	void DrawUI();

	
	
	
	
	void ChangeScene(const std::string& sceneName);

	
	void SetSceneFactory(AbstractSceneFactory* factory) { sceneFactory_ = factory; }

private:
	
	std::unique_ptr<BaseScene> nextScene_ = nullptr;
	
	std::unique_ptr<BaseScene> scene_ = nullptr;

	
	AbstractSceneFactory* sceneFactory_ = nullptr;
};

