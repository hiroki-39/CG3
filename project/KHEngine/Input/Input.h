#pragma once
#include<Windows.h>
#include<assert.h>
#include<wrl.h>
#define DIRECTLIB_VERSION 0x0800
#include<dinput.h>

#include "KHEngine/Core/OS/WinApp.h"

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

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

private: 

	//DirectInputのインスタンス
	ComPtr<IDirectInput8> directInput;

	//キーボード
	ComPtr<IDirectInputDevice8> keyboard;

	//キーの入力状態保存用配列
	BYTE key[256] = {};

	//前回のキーの入力状態保存用配列
	BYTE keyPre[256] = {};

private:
	//WindowAPI
	WinApp* winApp_ = nullptr;
};

