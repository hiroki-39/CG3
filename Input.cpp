#include "Input.h"

using namespace Microsoft::WRL;

void Input::Initialize(WinApp* winApp)
{
	/*--- キーボードの初期化 ---*/

	//WindowAPIのポインタを受け取る
	winApp_ = winApp;

	HRESULT result;

	//DirecctInputの初期化
	result = DirectInput8Create(winApp_->GetHInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, nullptr);
	assert(SUCCEEDED(result));

#pragma region "キーボード操作"

	//キーボードデバイスの生成
	result = directInput->CreateDevice(GUID_SysKeyboard, &keyboard, NULL);
	assert(SUCCEEDED(result));

	//入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(winApp_->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

#pragma endregion

#pragma region "コントロール操作"

	//キーボードデバイスの生成
	result = directInput->CreateDevice(GUID_SysKeyboard, keyboard.GetAddressOf(), nullptr);
	assert(SUCCEEDED(result));

	//入力データ形式のセット
	result = keyboard->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	//排他制御レベルのセット
	result = keyboard->SetCooperativeLevel(winApp_->GetHwnd(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

#pragma endregion
}

void Input::Update()
{
	HRESULT result;

	//前回のキーの入力状態を保存
	memcpy(keyPre, key, sizeof(key));

	//キーボードの入力情報取得
	keyboard->Acquire();

	//全キーの入力情報を取得
	keyboard->GetDeviceState(sizeof(key), key);
}

bool Input::PushKey(BYTE keyNumber)
{
	//キーが押されている場合はtrueを返す
	if (key[keyNumber])
	{
		return true;
	}

	//押されていない場合はfalseを返す
	return false;
}

bool Input::TriggerKey(BYTE keyNumber)
{
	//キーが押されていて、前回の状態が押されていない場合はトリガーと判断する
	if (key[keyNumber] && !keyPre[keyNumber])
	{
		return true;
	}

	//トリガーの条件を満たさないものはすべてfalseを返す
	return false;
}
