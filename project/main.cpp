#include "KHEngine/Core/Application/Application.h"
#include "KHEngine/Core/Framework/KHFramework.h"

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	KHFramework* game = new Application();

	game->Run();

	delete game;

	return 0;
}