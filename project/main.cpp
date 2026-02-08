#include "KHEngine/Core/Application/Application.h"
#include "KHEngine/Core/Framework/KHFramework.h"

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	Application* game = new Application();

	// ゲームの初期化
	game->Initialize();

	while (true) {
		// 毎フレーム処理
		game->Update();

		// ゲーム終了要求が来ていたらループを抜ける
		if (game->IsEndRequest()) {
			break;
		}

		game->Draw();
	}

	game->Finalize();

	return 0;
}