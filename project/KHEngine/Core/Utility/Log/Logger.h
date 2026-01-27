#pragma once
#include <string>

namespace Logger
{
	// 起動時に一度呼ぶ（ログファイル生成）
	void Initialize();

	// 終了時に一度呼ぶ（ファイルクローズ）
	void Shutdown();

	// ログ出力（スレッドセーフ）
	void Log(const std::string& message);
}