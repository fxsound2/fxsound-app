/* FxSound — Linux build: minimal stand-in for Windows <mmsystem.h>.
   Provides the WAVEFORMATEX layout the WAV utilities reference. */
#ifndef FXSOUND_DSP_FAKE_MMSYSTEM_H
#define FXSOUND_DSP_FAKE_MMSYSTEM_H

#include <windows.h>

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

typedef struct tWAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *LPWAVEFORMATEX;

typedef DWORD MMRESULT;

#endif
