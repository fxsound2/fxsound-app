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

#pragma once

#include <Windows.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#include <mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <comdef.h>

#include <string>
#include <vector>

#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <setupapi.h>
#include <regstr.h>
#include <infstr.h>
#include <cfgmgr32.h>
#include <string.h>
#include <malloc.h>
#include <newdev.h>
#include <objbase.h>
#include <strsafe.h>
#include <io.h>
#include <fcntl.h>

//
// exit codes
//
#define EXIT_OK      (0)
#define EXIT_REBOOT  (1)
#define EXIT_FAIL    (2)
#define EXIT_USAGE   (3)

//
// UpdateDriverForPlugAndPlayDevices
//
typedef BOOL(WINAPI *UpdateDriverForPlugAndPlayDevicesProto)(_In_opt_ HWND hwndParent,
    _In_ LPCTSTR HardwareId,
    _In_ LPCTSTR FullInfPath,
    _In_ DWORD InstallFlags,
    _Out_opt_ PBOOL bRebootRequired
    );
typedef BOOL(WINAPI *SetupSetNonInteractiveModeProto)(_In_ BOOL NonInteractiveFlag
    );
typedef BOOL(WINAPI *SetupUninstallOEMInfProto)(_In_ LPCTSTR InfFileName,
    _In_ DWORD Flags,
    _Reserved_ PVOID Reserved
    );

#if _SETUPAPI_VER >= _WIN32_WINNT_WINXP
typedef BOOL(WINAPI *SetupVerifyInfFileProto)(_In_ LPCTSTR InfName,
    _In_opt_ PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,
    _Inout_ PSP_INF_SIGNER_INFO InfSignerInfo);
#endif

#ifdef _UNICODE
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesW"
#define SETUPUNINSTALLOEMINF "SetupUninstallOEMInfW"
#else
#define UPDATEDRIVERFORPLUGANDPLAYDEVICES "UpdateDriverForPlugAndPlayDevicesA"
#define SETUPUNINSTALLOEMINF "SetupUninstallOEMInfA"
#endif
#define SETUPSETNONINTERACTIVEMODE "SetupSetNonInteractiveMode"
#define SETUPVERIFYINFFILE "SetupVerifyInfFile"

int cmdInstall(_In_opt_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_  LPCTSTR inf, _In_  LPCTSTR hwid);
int cmdUpdate(_In_opt_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_  LPCTSTR inf, _In_  LPCTSTR hwid);
int cmdRemove(_In_opt_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ LPCTSTR hwid);

class DfxInstall final
{
public:
	struct AudioDevice {
		std::wstring device_name;
		std::wstring device_guid;
		DWORD state = 0;
	};

	enum class CpuArch {Unknown=0, x86, x64, ARM64};

	DfxInstall(const wchar_t* working_dir, const wchar_t* version);
	~DfxInstall();

	bool InstallDFXDriver(std::string& log);
	bool UninstallDFXDriver(std::string& log);
    bool UninstallFxSoundDriver(std::string& log);
	bool CreateUpdateTask(std::string& log);
	bool DeleteUpdateTask(std::string& log);

private:
	static constexpr wchar_t DRIVERS_FOLDER[] = L"Drivers\\";
	static constexpr wchar_t BOOTSTRAP_FOLDER[] = L"Drivers\\bootstrap\\";
	static constexpr wchar_t APPS_FOLDER[] = L"Apps\\";
	static constexpr char VENDOR_CODE[] = "23";
	static constexpr wchar_t WVENDOR_CODE[] = L"23";

	static constexpr DWORD ENABLE_DEVICE = 0x1;
	static constexpr DWORD DISABLE_DEVICE = 0x10000001;
	static constexpr wchar_t REG_PATH_DEVICES[] = LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render\)";

	bool InstallIntelDriver(std::string& log);
	bool UninstallIntelDriver(std::string& log);
	bool InstallARMDriver(std::string& log);
	bool UninstallARMDriver(std::string& log);

	bool CmdExec(const std::wstring& cmd_str, const std::wstring& working_dir, std::string& output);
	bool FindDFXDriver(const std::wstring& version);
    bool IsWindows10();

	bool EnableDFXDriver();
	bool EnumAudioOutputs(std::vector<DfxInstall::AudioDevice>& audio_devices);
	std::wstring GetAudioOutputName(IMMDevice* p_device);

	std::wstring working_dir_;
	std::wstring version_;

	CpuArch cpu_arch_;
};

