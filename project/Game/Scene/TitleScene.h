#pragma once
#include <vector>
#include "KHEngine/Graphics/2d/Sprite.h"
#include "KHEngine/Core/Framework/BaseScene.h"

class TitleScene : public BaseScene
{
public:
    void Initialize();
    void Update();
    void Draw();
    void Finalize();

private:
};