/*
FxSound Linux port — compat/mmsystem.h stub (Phase 5)
PWAV legacy WinMM path is not ported in v1. This stub lets PWAV headers
parse on Linux; PWAV runtime is excluded from Linux build (see CMake).
Untested.
*/
#pragma once
#include "win_compat.h"
using HMMIO = void*;
using HWAVEIN = void*;
using HWAVEOUT = void*;
using HPSTR = char*;
struct WAVEFORMATEX { WORD wFormatTag; WORD nChannels; DWORD nSamplesPerSec; DWORD nAvgBytesPerSec; WORD nBlockAlign; WORD wBitsPerSample; WORD cbSize; };
struct WAVEHDR { char* lpData; DWORD dwBufferLength; DWORD dwBytesRecorded = 0; DWORD dwUser = 0; DWORD dwFlags = 0; DWORD dwLoops = 0; };
using LPWAVEHDR = WAVEHDR*;
struct MMCKINFO { DWORD ckid = 0; DWORD cksize = 0; DWORD fccType = 0; DWORD dwDataOffset = 0; DWORD dwFlags = 0; };
using LPMMCKINFO = MMCKINFO*;
