#include "KHEngine/Core/Utility/Log/Logger.h"
#include <Windows.h>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <memory>

namespace Logger
{
	namespace
	{
		std::mutex g_mutex;
		std::unique_ptr<std::ofstream> g_stream;

		std::string CreateLogFilePath()
		{
			::CreateDirectoryW(L"log", nullptr);

			auto now = std::chrono::system_clock::now();
			auto now_time_t = std::chrono::system_clock::to_time_t(now);
			std::tm local_tm;
			localtime_s(&local_tm, &now_time_t);

			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

			std::ostringstream oss;
			oss << std::put_time(&local_tm, "%Y%m%d_%H%M%S")
				<< '_' << std::setw(3) << std::setfill('0') << ms.count()
				<< '_' << GetCurrentProcessId()
				<< ".log";

			return std::string("log/") + oss.str();
		}

		void EnsureStream()
		{
			if (g_stream && g_stream->is_open()) return;
			std::string path = CreateLogFilePath();
			g_stream = std::make_unique<std::ofstream>(path.c_str(), std::ios::out | std::ios::app);
		}
	}

	void Init()
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		EnsureStream();
	}

	void Shutdown()
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		if (g_stream)
		{
			g_stream->flush();
			g_stream->close();
			g_stream.reset();
		}
	}

	void Log(const std::string& message)
	{
		std::lock_guard<std::mutex> lock(g_mutex);
		EnsureStream();
		if (!g_stream || !g_stream->is_open()) return;

		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		std::tm local_tm;
		localtime_s(&local_tm, &now_time_t);
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		std::ostringstream oss;
		oss << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S")
			<< '.' << std::setw(3) << std::setfill('0') << ms.count()
			<< " [" << GetCurrentThreadId() << "] "
			<< message << '\n';

		(*g_stream) << oss.str();
		g_stream->flush();

		// デバッグ出力にも送る（Visual Studio の出力ウィンドウ等）
		OutputDebugStringA(oss.str().c_str());
	}
}