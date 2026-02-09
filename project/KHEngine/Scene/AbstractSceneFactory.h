#pragma once
#include "KHEngine/Core/Framework/BaseScene.h"
#include <string>

class AbstractSceneFactory
{
public:
	virtual ~AbstractSceneFactory() = default;
	virtual BaseScene* CreateScene(const std::string& sceneName) = 0;
};

