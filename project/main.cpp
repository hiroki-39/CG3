#include "KHEngine/Core/Application/Application.h"
#include "KHEngine/Core/Framework/KHFramework.h"
#include <memory>

// windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    auto game = std::make_unique<Application>();

    game->Run();

    // 自動破棄
    return 0;
}