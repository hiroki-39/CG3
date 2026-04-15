#pragma once
#include<Windows.h>
#include<assert.h>
#include<wrl.h>
#define DIRECTLIB_VERSION 0x0800
#include<dinput.h>
#include<Xinput.h>

#include "KHEngine/Core/OS/WinApp.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"xinput.lib")

//入力
class Input
{
public:
	//namespaceの省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

	/// <summary>
	/// 全体の初期化
	/// </summary>
	/// <param name="WinApp"></param>
	void Initialize(WinApp* winApp);

	/// <summary>
	/// 更新処理
	/// </summary>
	void Update();

	/// <summary>
	/// キーの押下をチェック
	/// </summary>
	/// <param name="keyNumber">キーの番号</param>
	/// <returns>押されているかどうか</returns>
	bool PushKey(BYTE keyNumber);

	/// <summary>
	/// キーのトリガーをチェック
	/// </summary>
	/// <param name="keyNumber">キーの番号</param>
	/// <returns>トリガーかどうか</returns>
	bool TriggerKey(BYTE keyNumber);

	/* --- マウス --- */
	/// <summary>
	/// マウスボタンの押下チェック（button: 0～3）
	/// </summary>
	bool PushMouseButton(int button);

	/// <summary>
	/// マウスボタンのトリガー（押し始め）チェック
	/// </summary>
	bool TriggerMouseButton(int button);

	/// <summary>
	/// マウスの移動量（前フレームとの差分）
	/// </summary>
	LONG GetMouseMoveX() const;
	LONG GetMouseMoveY() const;
	LONG GetMouseWheel() const;

	/* --- コントローラー（XInput） --- */
	/// <summary>
	/// コントローラボタンの押下チェック（マスク：XINPUT_GAMEPAD_*）
	/// </summary>
	bool PushPadButton(WORD buttonMask);

	/// <summary>
	/// コントローラボタンのトリガー（押し始め）チェック
	/// </summary>
	bool TriggerPadButton(WORD buttonMask);


private:

	//DirectInputのインスタンス
	ComPtr<IDirectInput8> directInput;

	//キーボード
	ComPtr<IDirectInputDevice8> keyboard;

	//マウス
	ComPtr<IDirectInputDevice8> mouse;

	//キーの入力状態保存用配列
	BYTE key[256] = {};

	//前回のキーの入力状態保存用配列
	BYTE keyPre[256] = {};

	//マウス状態（DIMOUSESTATE2）
	DIMOUSESTATE2 mouseState = {};
	DIMOUSESTATE2 mouseStatePre = {};

	//XInputの状態
	XINPUT_STATE padState = {};
	XINPUT_STATE padStatePre = {};
	bool padConnected = false;

private:
	//WindowAPI
	WinApp* winApp_ = nullptr;
};

