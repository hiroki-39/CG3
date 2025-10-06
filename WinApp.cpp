#include "WinApp.h"

//ウインドウプロシージャ
LRESULT WinApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparm)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparm))
	{
		return true;
	}

	//メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		//ウィンドウが壊された
	case WM_DESTROY:

	//OSに対して、アプリの終了を伝える
	PostQuitMessage(0);

	return 0;
	}

	//標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparm);
}

void WinApp::Initialize()
{
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);

	//ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	//ウィンドウクラス名
	wc.lpszClassName = L"CG2MyWindowClass";
	//インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	//カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	//ウィンドウクラスの登録
	RegisterClass(&wc);

	//ウィンドサイズを表す構造体にクライアント領域のサイズを入れる
	RECT wrc = { 0, 0, kClientWidth, kClientHeight };

	//クライアント領域のサイズをウィンドウサイズに変換する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	//ウィンドウの生成
	hwnd = CreateWindow(
		wc.lpszClassName,     //クラス名
		L"GE3_LE2B_04_カトウ_ヒロキ",       //タイトルバー名
		WS_OVERLAPPEDWINDOW,  //ウィンドウスタイル
		CW_USEDEFAULT,        //表示座X標(Windowsに任せる)
		CW_USEDEFAULT,        //表示Y座標(WindowsOSに任せる)
		wrc.right - wrc.left, //ウィンドウ横幅
		wrc.bottom - wrc.top, //ウィンドウ縦幅
		nullptr,              //親ウィンドウハンドル
		nullptr,              //メニューハンドル
		wc.hInstance,         //インスタンスハンドル
		nullptr               //オプション
	);

	//ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);
}

void WinApp::Update()
{


	/*Log(logStream, "Hello DirectX!\n");
	Log(logStream, ConvertString(std::format(L"clientSize:{},{}\n", kClientWidth, kClientHeight)));*/
}

void WinApp::Finalize()
{
	CloseWindow(hwnd);

	//COMの終了処理
	CoUninitialize();
}

bool WinApp::ProcessMessage()
{
	MSG msg{};

	//windowsにメッセージが来たら最優先で処理する
	if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//もし終了メッセージが来ていたらtrueを返す
	if (msg.message == WM_QUIT)
	{
		return true;
	}

	return false;
}
