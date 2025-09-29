#include "Engine.h"

void Engine::Initialize(HWND hwnd)
{
    // デバイス初期化、モデル/マテリアルの読み込みなど
}

void Engine::Update()
{
    camera_.Update();
    model_.Update();
}

void Engine::Draw()
{
    model_.Draw(commandList_.Get(), camera_, light_);
}

void Engine::Finalize()
{
    soundManager_.Finalize();
}

