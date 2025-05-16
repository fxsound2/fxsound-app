/*
FxSound
Copyright (C) 2025  FxSound LLC

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

void RunAudioDiagnostics();
void EnableVirtualTerminalProcessing();

int wmain(int argc, wchar_t* argv[])
{
	HRESULT hr = S_OK;

    EnableVirtualTerminalProcessing();

	hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
		return -1;

    RunAudioDiagnostics();

	CoUninitialize();

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

void RunAudioDiagnostics()
{
	auto audioDevices = EnumAudioDevices();
	ReportAudioDevices(audioDevices);

	auto audioSessions = EnumAudioSessions();
	ReportAudioSessions(audioSessions);
}
