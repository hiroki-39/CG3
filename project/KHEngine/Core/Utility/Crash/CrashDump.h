#pragma once
#include <Windows.h>

namespace KHEngine
{
	namespace Core
	{
		namespace Utility
		{
			namespace Crash
			{
				class CrashDump
				{
				public:
					// 起動時に一度インストール
					static void Install();

					// 終了時にアンインストール（任意）
					static void Uninstall();

				private:
					static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);
					static LPTOP_LEVEL_EXCEPTION_FILTER previousFilter_;
				};
			}
		}
	}
}