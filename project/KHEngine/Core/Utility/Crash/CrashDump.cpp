#include "KHEngine/Core/Utility/Crash/CrashDump.h"
#include "KHEngine/Core/Utility/Log/Logger.h"
#include <dbghelp.h>
#include <strsafe.h>

#pragma comment(lib, "Dbghelp.lib")

namespace KHEngine
{
	namespace Core
	{
		namespace Utility
		{
			namespace Crash
			{
				LPTOP_LEVEL_EXCEPTION_FILTER CrashDump::previousFilter_ = nullptr;

				void CrashDump::Install()
				{
					previousFilter_ = SetUnhandledExceptionFilter(ExportDump);
				}

				void CrashDump::Uninstall()
				{
					SetUnhandledExceptionFilter(previousFilter_);
				}

				LONG WINAPI CrashDump::ExportDump(EXCEPTION_POINTERS* exception)
				{
					SYSTEMTIME time;
					::GetLocalTime(&time);

					wchar_t filePath[MAX_PATH] = { 0 };
					::CreateDirectoryW(L"./Dumps", nullptr);
					StringCchPrintfW(filePath, MAX_PATH,
						L"./Dumps/%04d%02d%02d_%02d%02d%02d_pid%u_tid%u.dmp",
						time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond,
						::GetCurrentProcessId(), ::GetCurrentThreadId());

					HANDLE dumpFileHandle = ::CreateFileW(filePath, GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ,
						nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

					if (dumpFileHandle != INVALID_HANDLE_VALUE)
					{
						MINIDUMP_EXCEPTION_INFORMATION info{};
						info.ThreadId = ::GetCurrentThreadId();
						info.ExceptionPointers = exception;
						info.ClientPointers = TRUE;

						BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFileHandle,
							MiniDumpNormal, &info, nullptr, nullptr);

						::CloseHandle(dumpFileHandle);

						if (ok)
						{
							std::wstring wpath(filePath);
							std::string path;
							path.assign(wpath.begin(), wpath.end());
							Logger::Log(std::string("Crash dump written: ") + path);
						}
						else
						{
							Logger::Log("Failed to write crash dump.");
						}
					}
					else
					{
						Logger::Log("Failed to create dump file handle.");
					}

					return EXCEPTION_EXECUTE_HANDLER;
				}
			}
		}
	}
}