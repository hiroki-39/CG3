#include "KHEngine/Core/Application/Application.h"

//windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	// アプリケーションの初期化
	Application app;

	// アプリケーションの初期化
	app.Initialize();

	// ゲームループ
	while (true)
	{
		app.Update();

		// 終了リクエストが来ていたらループを抜ける
		if (app.IsEndRequest())
		{
			break;
		}

		app.Draw();
	}

	app.Finalize();

	return 0;
}