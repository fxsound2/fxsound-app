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
#include "codedefs.h"

/* Standard includes */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include <Shellapi.h>
#include <strsafe.h>

#include "slout.h"
#include "operatingSystem.h"
#include "reg.h"
#include "versionhelpers.h"

#define OPERATING_SYSTEM_REG_STR_SIZE           512

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;


/*
 * FUNCTION: operatingSystemIsVistaOrHigher()
 * DESCRIPTION:
 *  Determines and passes whether or not the os version is vista or higher.
 *
 */
int PT_DECLSPEC operatingSystemIsVistaOrHigher(int *ip_os_vista_or_higher)
{
	bool b_is_vista_or_greater;

	b_is_vista_or_greater = IsWindowsVistaOrGreater();

	*ip_os_vista_or_higher = (int)b_is_vista_or_greater;

   return(OKAY);
}

/*
 * FUNCTION: operatingSystemIsXPOrHigher()
 * DESCRIPTION:
 *  Determines and passes whether or not the os version is XP or higher.
 *
 */
int PT_DECLSPEC operatingSystemIsXPOrHigher(int *ip_os_xp_or_higher)
{
	bool b_is_xp_or_greater;

	b_is_xp_or_greater = IsWindowsXPOrGreater();

	*ip_os_xp_or_higher = (int)b_is_xp_or_greater;
	
   return(OKAY);
}

/*
 * FUNCTION: operatingSystemIsWMP9orHigherInstalled()
 * DESCRIPTION:
 *
 */
int PT_DECLSPEC operatingSystemIsWMP9orHigherInstalled(int *ip_wmp9_or_higher_installed)
{
	int wmp_version;
	int key_exists;
	unsigned long ul_wmp_is_installed;
	wchar_t wcp_key_value[OPERATING_SYSTEM_REG_STR_SIZE];

   *ip_wmp9_or_higher_installed = IS_FALSE;

   /* Check if WMP is installed */
   if (regReadKeyWithKeyname_Dword_Wide(REG_LOCAL_MACHINE, 
		    L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{6BF52A52-394A-11d3-B153-00C04F79FAA6}", 
			 L"IsInstalled", &key_exists, &ul_wmp_is_installed) != OKAY)
	   return(NOT_OKAY);

	/* If the key does not exist, then the WMP must not be installed */
	if (!key_exists)
		return(OKAY);

	if (ul_wmp_is_installed <= 0L)
      return(OKAY);
			
   /* Get the WMP version number */
	if (regReadKeyWithKeyname_String_Wide(REG_LOCAL_MACHINE,
		    L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components\\{6BF52A52-394A-11d3-B153-00C04F79FAA6}", 
			 L"Version", &key_exists, wcp_key_value, (unsigned long)OPERATING_SYSTEM_REG_STR_SIZE) != OKAY)
	   return(NOT_OKAY);

	if (!key_exists)
		return(OKAY);

	if (wcslen(wcp_key_value) <= 0)
		return(OKAY);

	swscanf(wcp_key_value, L"%d", &wmp_version);

	if (wmp_version < 9)
		return(OKAY);

   *ip_wmp9_or_higher_installed = IS_TRUE;

	return(OKAY);
}

/*
 * FUNCTION: operatingSystemIsWow64()
 * DESCRIPTION:
 *
 *  Passes back IS_TRUE if the current process is compiled as 32 bit, and is
 *  running on a 64 bit machine.  That state is known as Windows on Windows (WOW64).
 *
 */
int PT_DECLSPEC operatingSystemIsWow64(int *ip_is_wow64)
{
	BOOL bIsWow64;

	*ip_is_wow64 = IS_FALSE;

	/* 
	 * Check if the IsWow64 system check is available.  
	 *
	 * Note: This function did not exist in pre-XP version of the OS, so
	 * we need to dynamically get the function pointer.
	 */
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")),"IsWow64Process");
  
	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
			return(NOT_OKAY);

		if (bIsWow64)
			*ip_is_wow64 = IS_TRUE;
    }

    return(OKAY);
}

/*
 * FUNCTION: operatingSystemGetDiskInfo()
 * DESCRIPTION:
 *  Given a valid directionry (UNC is okay), it returns the total and free space in bytes in the drive.
 *
 */
int PT_DECLSPEC operatingSystemGetDiskInfo(wchar_t *wcp_directory, unsigned __int64 *ip64_total_space_in_bytes, 
														 unsigned __int64 *ip64_free_space_in_bytes, CSlout *hp_slout)
{
	unsigned __int64 i64FreeBytes;
	wchar_t wcp_drive_letter[8];
	wchar_t wcp_file_msg[128];

	*ip64_total_space_in_bytes = 0;
	*ip64_free_space_in_bytes = 0;

	if (wcp_directory == NULL)
		return(NOT_OKAY);

	if (GetDiskFreeSpaceEx(wcp_directory, (PULARGE_INTEGER)ip64_free_space_in_bytes, (PULARGE_INTEGER)ip64_total_space_in_bytes, (PULARGE_INTEGER)&i64FreeBytes) == 0)
	{
		if (hp_slout != NULL)
		{
		   swprintf(wcp_file_msg, L"            operatingSystemGetDiskInfo: GetDiskFreeSpaceEx() fails on %s drive", wcp_drive_letter);
		   hp_slout->Message_Wide(FIRST_LINE, wcp_file_msg);
		}
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: operatingSystemGetMemoryInfo()
 * DESCRIPTION:
 *
 */
int PT_DECLSPEC operatingSystemGetMemoryInfo(unsigned long *lp_percent_in_use, 
															unsigned __int64 *ip64_total_physical_in_bytes,
															unsigned __int64 *ip64_free_physical_memory_in_bytes,
															unsigned __int64 *ip64_total_paging_in_bytes,
															unsigned __int64 *ip64_free_paging_in_bytes,
															unsigned __int64 *ip64_total_virtual_in_bytes,
															unsigned __int64 *ip64_free_virtual_in_bytes,
															unsigned __int64 *ip64_free_extended_in_bytes,
														   CSlout *hp_slout)
{
	MEMORYSTATUSEX statex;

	*lp_percent_in_use = 0;
	*ip64_total_physical_in_bytes = 0;
	*ip64_free_physical_memory_in_bytes = 0;
	*ip64_total_paging_in_bytes = 0;
	*ip64_free_paging_in_bytes = 0;
	*ip64_total_virtual_in_bytes = 0;
	*ip64_free_virtual_in_bytes = 0;
	*ip64_free_extended_in_bytes = 0;

	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex) == 0)
		return(NOT_OKAY);

	*lp_percent_in_use = statex.dwMemoryLoad;
	*ip64_total_physical_in_bytes = statex.ullTotalPhys;
	*ip64_free_physical_memory_in_bytes = statex.ullAvailPhys;
	*ip64_total_paging_in_bytes = statex.ullTotalPageFile;
	*ip64_free_paging_in_bytes = statex.ullAvailPageFile;
	*ip64_total_virtual_in_bytes = statex.ullTotalVirtual;
	*ip64_free_virtual_in_bytes = statex.ullAvailVirtual;
	*ip64_free_extended_in_bytes = statex.ullAvailExtendedVirtual;

	return(OKAY);
}

/*
 * FUNCTION: operatingSystemGetMemoryInfo()
 * DESCRIPTION:
 *
 *  Run the exe specified by the passed fullpath.  This function is useful
 *  for firing up installers.  In Vista and higher operating systems we must fire up
 *  the installer this way because an elevated privilege is needed.
 *
 */
int PT_DECLSPEC operatingSystemRunExeWithElevatedPrivilegeIfNecessary(
															wchar_t *wcp_exe_fullpath,
														   CSlout *hp_slout)
{
	int i_os_vista_or_higher;

	/* Determine if OS is vista or higher */
	if (operatingSystemIsVistaOrHigher(&i_os_vista_or_higher) != OKAY)
		return(NOT_OKAY);

	if (i_os_vista_or_higher)
	{
		SHELLEXECUTEINFO TempInfo = {0};

		TempInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		TempInfo.fMask = 0;
		TempInfo.hwnd = NULL;
		TempInfo.lpVerb = L"runas";
		TempInfo.lpFile = wcp_exe_fullpath;
		TempInfo.lpParameters = L"";
		TempInfo.lpDirectory = NULL;
		TempInfo.nShow = SW_NORMAL;

		::ShellExecuteEx(&TempInfo);
	}
	else
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		// Initialize the startup info struct
		GetStartupInfo(&si);

		// Call CreateProcess(), mostly defaults.  Note that
		// the lpCommandLine argument MUST NOT be a constant string.
		CreateProcess(NULL, wcp_exe_fullpath, NULL, NULL, FALSE, NULL,
              NULL, NULL, &si, &pi);

		// Close our reference to the initial thread
		CloseHandle(pi.hThread);

		// Wait for the process to exit.
		//WaitForSingleObject(pi.hProcess, INFINITE);

		// And close our reference to the process.
		CloseHandle(pi.hProcess);
	}

	return(OKAY);
}

/*
 * FUNCTION: operatingSystemGetScreenWorkAreaResolution()
 * DESCRIPTION:
 *
 *  Passes back the screen resolution of the primary screen.
 *
 */
int PT_DECLSPEC operatingSystemGetScreenWorkAreaResolution(int *ip_screen_width, int *ip_screen_height, CSlout *hp_slout)
{
	RECT rect_screen;

	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rect_screen, 0) == 0)
		return(NOT_OKAY);

	*ip_screen_width = rect_screen.right - rect_screen.left;
	*ip_screen_height = rect_screen.bottom - rect_screen.top;

	return(OKAY);
}

/*
 * FUNCTION: operatingSystemIsIdle()
 * DESCRIPTION:
 *
 *  Passes back whether or not the system has been idle.  If the mouse has not been moved and no key has been
 *  pressed for the passed number of seconds, then it is considered idle.
 *
 */
int PT_DECLSPEC operatingSystemIsIdle(int i_min_secs_to_consider_idle, int *ip_is_idle, CSlout *hp_slout)
{
	LASTINPUTINFO lif;
	int i_system_up_msecs;
	int i_last_event_tick_count;
	int i_idle_num_secs;
	int ret_value;

	*ip_is_idle = IS_FALSE;
 
	lif.cbSize = sizeof(lif);
	lif.dwTime = 0;

	/* Retrieves the time of the last input event. */
	ret_value = GetLastInputInfo(&lif);

	if (ret_value == 0)
		return(NOT_OKAY);

	/* Get the tick count of last input event */
	i_last_event_tick_count = (int)lif.dwTime;

	/* Retrieves the number of milliseconds that have elapsed since the system was started, up to 49.7 days. */
	i_system_up_msecs = (int)GetTickCount();

	/* Calculate how many seconds the system has been idle */
	i_idle_num_secs = (i_system_up_msecs - i_last_event_tick_count) / 1000;

	/* Check if enough time has passed to be considered idle */
	if (i_idle_num_secs >= i_min_secs_to_consider_idle)
	{
		*ip_is_idle = IS_TRUE;
	}

	return(OKAY);
}

/*
 * FUNCTION: operatingSystemGetSystemProperties()
 * DESCRIPTION: Gets the current Windows version, CPU type (32 bit or 64 bit) and number of processor cores.
 */
int PT_DECLSPEC operatingSystemGetSystemProperties(unsigned int *uipProps, int *ipNumCores)
{
	typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
	typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);

   OSVERSIONINFOEX osvi;
   SYSTEM_INFO si;
   PGNSI pGNSI;

	*uipProps = 0;

   ZeroMemory(&si, sizeof(SYSTEM_INFO));
   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   // For compatibilty when running under SYSWOW64, call GetNativeSystemInfo if available or GetSystemInfo otherwise.
   pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");

   if(NULL != pGNSI)
      pGNSI(&si);
   else GetSystemInfo(&si);

	if( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) // This means a 64 bit OS
	{
		if( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 2) )
			*uipProps |= OPERATING_SYSTEM_WIN8_64;

		else if( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 1) )
			*uipProps |= OPERATING_SYSTEM_WIN7_64;

		else if( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 0) )
			*uipProps |= OPERATING_SYSTEM_VISTA_64;

		else if( osvi.dwMajorVersion < 6 )
			*uipProps |= OPERATING_SYSTEM_XP;
	}
	else // Its a 32 bit OS
	{
		if( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 2) )
			*uipProps |= OPERATING_SYSTEM_VISTA_32;

		else if( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 1) )
			*uipProps |= OPERATING_SYSTEM_WIN7_32;

		else if( (osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 0) )
			*uipProps |= OPERATING_SYSTEM_VISTA_32;

		else if( osvi.dwMajorVersion < 6 )
			*uipProps |= OPERATING_SYSTEM_XP;
	}

	// Now and in the hardware type
	if( si.dwProcessorType <= 586 )
		*uipProps |= OPERATING_SYSTEM_CPU_32;
	else
		*uipProps |= OPERATING_SYSTEM_CPU_64;

	*ipNumCores = si.dwNumberOfProcessors;

	return(OKAY);
}

/*
* FUNCTION: operatingSystemGetLastErrorMessage()
* DESCRIPTION: Retrieve the system error message for the last-error code https://msdn.microsoft.com/en-us/library/windows/desktop/ms680582(v=vs.85).aspx
*/
int PT_DECLSPEC operatingSystemGetLastErrorMessage(DWORD dwLastErrorCode, wchar_t* wcpLastErrorMessage)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	//DWORD dw = GetLastError();
	DWORD dw = dwLastErrorCode;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)L"WOW") + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("Error code %d: %s"),
		dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
	/////////////////////////
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwLastErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		wcpLastErrorMessage,
		0, NULL);


	return(OKAY);
}