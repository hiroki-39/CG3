#include "Engine.h"

void Engine::Initialize(HWND hwnd)
{
    // �f�o�C�X�������A���f��/�}�e���A���̓ǂݍ��݂Ȃ�
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

