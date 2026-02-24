#pragma once
#include <memory>
#include <string>
#include "KHEngine/Core/Framework/BaseScene.h"

class AbstractSceneFactory
{
public:
	virtual ~AbstractSceneFactory() = default;
	virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};

