#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>

namespace Logger
{
	void Log(const std::string& message)
	{
		//ログのディレクトリを用意
		std::filesystem::create_directory("log");

		//現在時刻を取得(UTC時刻)
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

		//ログファイルの名前にコンマ何秒はいらないので、秒のみにする
		std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
			nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
		//日本時間(pcの設定時間)に変換
		std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
		//format関数で、年月日_時分秒の文字列に変換
		std::string datestring = std::format("{:%Y%m%d_%H%M%S}", localTime);
		//時刻を使ってファイル名を作成
		std::string logFilePath = std::format("log/") + datestring + ".log";
		//ファイルを作って書き込み準備
		std::ofstream logStream(logFilePath);
	}
}