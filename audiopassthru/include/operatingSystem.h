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
#ifndef _OPERATING_SYSTEM_H_
#define _OPERATING_SYSTEM_H_

#include "pt_defs.h"
#include "slout.h" 

/* Operating system and Cpu type defines */
#define OPERATING_SYSTEM_XP			1
#define OPERATING_SYSTEM_VISTA_32	2
#define OPERATING_SYSTEM_VISTA_64	4
#define OPERATING_SYSTEM_WIN7_32	8
#define OPERATING_SYSTEM_WIN7_64	16
#define OPERATING_SYSTEM_WIN8_32	32
#define OPERATING_SYSTEM_WIN8_64	64
#define OPERATING_SYSTEM_CPU_32		128
#define OPERATING_SYSTEM_CPU_64		256
#define OPERATING_SYSTEM_VISTA (OPERATING_SYSTEM_VISTA_32 | OPERATING_SYSTEM_VISTA_64)
#define OPERATING_SYSTEM_WIN7 (OPERATING_SYSTEM_WIN7_32 | OPERATING_SYSTEM_WIN7_64)
#define OPERATING_SYSTEM_WIN8 (OPERATING_SYSTEM_WIN8_32 | OPERATING_SYSTEM_WIN8_64)
#define OPERATING_SYSTEM_32_BIT_OS (OPERATING_SYSTEM_VISTA_32 | OPERATING_SYSTEM_WIN7_32 | OPERATING_SYSTEM_WIN8_32)
#define OPERATING_SYSTEM_64_BIT_OS (OPERATING_SYSTEM_VISTA_64 | OPERATING_SYSTEM_WIN7_64 | OPERATING_SYSTEM_WIN8_64)

/* MUTE SYSTEM SOUND DEFINES */
#define OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_APP_STARTUP		1
#define OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_RECORDING_START	2
#define OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_RECORDING_STOP	3
#define OPERATING_SYSTEM_MUTE_SOUNDS_LOCAL_REG_KEYNAME_WIDE     L"current_system_sound"
#define OPERATING_SYSTEM_MUTE_SOUNDS_RESTORED_VALUE_WIDE        L"restored"
#define OPERATING_SYSTEM_MUTE_SOUNDS_SYSTEM_REGISTRY_PATH       L"AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current"

/* MOUSE DEFINES */
#define OPERATION_SYSTEM_MOUSE_BUTTON_LEFT	1
#define OPERATION_SYSTEM_MOUSE_BUTTON_RIGHT	2

/* operatingSystem.cpp */
int PT_DECLSPEC operatingSystemIsVistaOrHigher(int *);
int PT_DECLSPEC operatingSystemIsXPOrHigher(int *);
int PT_DECLSPEC operatingSystemIsWMP9orHigherInstalled(int *);
int PT_DECLSPEC operatingSystemIsWow64(int *);
int PT_DECLSPEC operatingSystemGetDiskInfo(wchar_t *, unsigned __int64 *, unsigned __int64 *, CSlout *);
int PT_DECLSPEC operatingSystemGetMemoryInfo(unsigned long *, unsigned __int64 *, unsigned __int64 *, unsigned __int64 *, unsigned __int64 *, unsigned __int64 *, unsigned __int64 *, unsigned __int64 *, CSlout *);
int PT_DECLSPEC operatingSystemRunExeWithElevatedPrivilegeIfNecessary(wchar_t *, CSlout *);
int PT_DECLSPEC operatingSystemGetScreenWorkAreaResolution(int *, int *, CSlout *);
int PT_DECLSPEC operatingSystemIsIdle(int, int *, CSlout *);
int PT_DECLSPEC operatingSystemGetSystemProperties(unsigned int *, int *);
int PT_DECLSPEC operatingSystemGetLastErrorMessage(DWORD, wchar_t*);

/* operatingSystemMouse.cpp */
int PT_DECLSPEC operatingSystemIsMouseButtonDown(int, int *);

/* operatingSystemSounds.cpp */
int PT_DECLSPEC operatingSystemMuteSystemSoundsForRecording(int, wchar_t *);

#endif //_OPERATING_SYSTEM_H_