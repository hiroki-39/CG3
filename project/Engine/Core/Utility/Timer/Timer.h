#pragma once
#include <chrono>

class Timer
{
public://メンバ関数

	/// <summary>
	/// 固定FPS制御の初期化
	/// </summary>
	void InitalizeFixFPS();

	/// <summary>
	/// 固定FPS制御の更新
	/// </summary>
	void UpdateFixFPS();

private://メンバ変数

	// 記録時間(FPS固定用)
	std::chrono::steady_clock::time_point reference_;

};

