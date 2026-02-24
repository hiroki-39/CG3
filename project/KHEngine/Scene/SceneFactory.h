#pragma once
#include <memory>
#include "KHEngine/Scene/AbstractSceneFactory.h"

class SceneFactory : public AbstractSceneFactory
{
public:
	/// <summary>
	/// シーン生成
	/// </summary>
	/// <param name="sceneName">シーン名</param>
	/// <returns>生成したシーン（所有権を渡す）</returns>
	std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) override;
};

