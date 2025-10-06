#pragma once
#include<Windows.h>

class WinApp
{
public:

	static LRESULT CALLBACK WindowProc(HWND hand, UINT msg, WPARAM wparam, LPARAM lparm);

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	//getter
	HWND GetHwnd() { return hwnd; }

	HINSTANCE GetHInstance() const { return wc.hInstance; }

public://定数

	//クライアント領域のサイズ
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

private:

	//ウィンドウのハンドル
	HWND hwnd = nullptr;

	//ウィンドウクラスの設定
	WNDCLASS wc{};
};

