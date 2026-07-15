/*
FxSound
Copyright (C) 2026  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// fxdiag.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "fxdiag.h"
#include "AudioDevice.h"
#include "AudioSession.h"

void RunAudioDiagnostics(bool jsonMode);
void EnableVirtualTerminalProcessing();
bool ShouldPauseOnExit();
std::wstring SaveJsonReport(const std::wstring& json);

int wmain(int argc, wchar_t* argv[])
{
	HRESULT hr = S_OK;

	bool jsonMode = false;
	for (int i = 1; i < argc; i++)
	{
		if (_wcsicmp(argv[i], L"--json") == 0)
		{
			jsonMode = true;
		}
	}

	if (!jsonMode)
	{
		EnableVirtualTerminalProcessing();
	}

	hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
		return -1;

    RunAudioDiagnostics(jsonMode);

	CoUninitialize();

	if (!jsonMode && ShouldPauseOnExit())
	{
		wprintf(L"\nPress any key to exit...");
		_getch();
	}

	return 0;
}

void EnableVirtualTerminalProcessing() 
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hConsole == INVALID_HANDLE_VALUE) 
    {
        return;
    }

    DWORD consoleMode;
    if (!GetConsoleMode(hConsole, &consoleMode)) 
    {
        return;
    }

    // Enable the Virtual Terminal Processing flag for ANSI color support
    consoleMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, consoleMode);
}

bool ShouldPauseOnExit()
{
	DWORD processList[2];
	DWORD numProcesses = GetConsoleProcessList(processList, 2);

	// If this process is the only one attached to the console, the console
	// was created just for it (e.g. double-clicked from Explorer), so it
	// would otherwise close immediately. If another process (a shell) is
	// also attached, the console will stay open on its own after we exit.
	return numProcesses <= 1;
}

void RunAudioDiagnostics(bool jsonMode)
{
	auto audioDevices = EnumAudioDevices();
	auto audioSessions = EnumAudioSessions();

	std::wstring jsonReport = L"{\"devices\":" + ReportAudioDevicesJson(audioDevices)
		+ L",\"sessions\":" + ReportAudioSessionsJson(audioSessions)
		+ L"}";

	if (jsonMode)
	{
		std::wcout << jsonReport;
	}
	else
	{
		ReportAudioDevices(audioDevices);
		ReportAudioSessions(audioSessions);
	}

	std::wstring savedPath = SaveJsonReport(jsonReport);
	if (!jsonMode && !savedPath.empty())
	{
		std::wcout << std::endl << L"Diagnostics report saved to " << savedPath << std::endl;
	}
}

std::wstring SaveJsonReport(const std::wstring& json)
{
	wchar_t appDataPath[MAX_PATH];
	DWORD length = GetEnvironmentVariableW(L"APPDATA", appDataPath, MAX_PATH);
	if (length == 0 || length >= MAX_PATH)
	{
		return L"";
	}

	std::wstring folderPath = std::wstring(appDataPath) + L"\\FxSound";
	if (!CreateDirectoryW(folderPath.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		return L"";
	}

	std::wstring filePath = folderPath + L"\\fxdiag.json";

	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, NULL, 0, NULL, NULL);
	if (utf8Length <= 1)
	{
		return L"";
	}

	std::vector<char> utf8Json(utf8Length);
	WideCharToMultiByte(CP_UTF8, 0, json.c_str(), -1, utf8Json.data(), utf8Length, NULL, NULL);

	std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		return L"";
	}
	file.write(utf8Json.data(), utf8Length - 1);

	return filePath;
}
