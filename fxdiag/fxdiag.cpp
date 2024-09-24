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
