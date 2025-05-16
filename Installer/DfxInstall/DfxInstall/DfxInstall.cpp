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

#include "stdafx.h"
#include "DfxInstall.h"
#include <msiquery.h>
#pragma comment(lib, "Msi.lib")

UINT __stdcall DfxInstallLog(MSIHANDLE h_install, LPCSTR message);

extern "C" __declspec(dllexport) UINT __stdcall InstallDFXDriver(MSIHANDLE h_install)
{
    wchar_t params[MAX_PATH + 100] = {};
    DWORD dw_len;

    dw_len = _countof(params);
    UINT res = MsiGetProperty(h_install, TEXT("CustomActionData"), params, &dw_len);
    if (res != ERROR_SUCCESS)
    {
        return res;
    }

    std::wstring app_dir;
    std::wstring dfx_version;
    auto pos = wcschr(params, L';');
    if (pos != nullptr)
    {
        dfx_version = pos + 1;
        *pos = L'\0';
        app_dir = params;
    }
    else
    {
        return ERROR_GEN_FAILURE;
    }

    DfxInstall dfx_install(app_dir.c_str(), dfx_version.c_str());
    std::string log = {};
    if (!dfx_install.InstallDFXDriver(log))
    {
        DfxInstallLog(h_install, log.c_str());
        return ERROR_GEN_FAILURE;
    }

    DfxInstallLog(h_install, log.c_str());

    return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall UninstallDFXDriver(MSIHANDLE h_install)
{
    wchar_t app_dir[MAX_PATH] = {};
    DWORD dw_len;
    
    dw_len = _countof(app_dir);
    UINT res = MsiGetProperty(h_install, TEXT("CustomActionData"), app_dir, &dw_len);
    if (res != ERROR_SUCCESS)
    {
        return res;
    }

    DfxInstall dfx_install(app_dir, L"");
    std::string log = {};
    if (!dfx_install.UninstallDFXDriver(log))
    {
        DfxInstallLog(h_install, log.c_str());
        return ERROR_GEN_FAILURE;
    }

    DfxInstallLog(h_install, log.c_str());

    return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall UninstallFxSoundDriver(MSIHANDLE h_install)
{
    wchar_t app_dir[MAX_PATH] = {};
    DWORD dw_len;

    dw_len = _countof(app_dir);
    UINT res = MsiGetProperty(h_install, TEXT("CustomActionData"), app_dir, &dw_len);
    if (res != ERROR_SUCCESS)
    {
        return res;
    }

    DfxInstall dfx_install(app_dir, L"");
    std::string log = {};
    if (!dfx_install.UninstallFxSoundDriver(log))
    {
        DfxInstallLog(h_install, log.c_str());
        return ERROR_GEN_FAILURE;
    }

    DfxInstallLog(h_install, log.c_str());

    return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall CreateUpdateTask(MSIHANDLE h_install)
{
    wchar_t app_dir[MAX_PATH] = {};
    DWORD dw_len;

    dw_len = _countof(app_dir);
    UINT res = MsiGetProperty(h_install, TEXT("CustomActionData"), app_dir, &dw_len);
    if (res != ERROR_SUCCESS)
    {
        return res;
    }

    DfxInstall dfx_install(app_dir, L"");
    std::string log = {};
    if (!dfx_install.CreateUpdateTask(log))
    {
        DfxInstallLog(h_install, log.c_str());
        return ERROR_GEN_FAILURE;
    }

    DfxInstallLog(h_install, log.c_str());

    return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) UINT __stdcall DeleteUpdateTask(MSIHANDLE h_install)
{
    wchar_t app_dir[MAX_PATH] = {};
    DWORD dw_len;

    dw_len = _countof(app_dir);
    UINT res = MsiGetProperty(h_install, TEXT("CustomActionData"), app_dir, &dw_len);
    if (res != ERROR_SUCCESS)
    {
        return res;
    }

    DfxInstall dfx_install(app_dir, L"");
    std::string log = {};
    if (!dfx_install.DeleteUpdateTask(log))
    {
        DfxInstallLog(h_install, log.c_str());
        return ERROR_GEN_FAILURE;
    }

    DfxInstallLog(h_install, log.c_str());

    return ERROR_SUCCESS;
}

UINT __stdcall DfxInstallLog(MSIHANDLE h_install, LPCSTR message)
{
    PMSIHANDLE h_record = MsiCreateRecord(1);

    MsiRecordSetStringA(h_record, 0, "Dfx Log: [1]");
    MsiRecordSetStringA(h_record, 1, message);

    MsiProcessMessage(h_install, INSTALLMESSAGE_INFO, h_record);

    return ERROR_SUCCESS;
}

DfxInstall::DfxInstall(const wchar_t* working_dir, const wchar_t* version)
{
    _ASSERT(working_dir != nullptr);

    CoInitialize(NULL);

    working_dir_ = working_dir;
    version_ = version;

    SYSTEM_INFO sys_info;
    GetNativeSystemInfo(&sys_info);

    cpu_arch_ = CpuArch::Unknown;
    if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
    {
        cpu_arch_ = CpuArch::x86;
    }
    else if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
    {
        cpu_arch_ = CpuArch::x64;
    }
    else if (sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
    {
        cpu_arch_ = CpuArch::ARM64;
    }
}

DfxInstall::~DfxInstall()
{
    CoUninitialize();
}

bool DfxInstall::InstallDFXDriver(std::string& log)
{
    if (cpu_arch_ == CpuArch::Unknown)
    {
        return false;
    }

    if (cpu_arch_ == CpuArch::ARM64)
    {
        return InstallARMDriver(log);
    }
    else
    {
        return InstallIntelDriver(log);
    }
}

bool DfxInstall::UninstallDFXDriver(std::string& log)
{
    if (cpu_arch_ == CpuArch::Unknown)
    {
        return false;
    }
    if (working_dir_.empty())
    {
        return false;
    }

    if (working_dir_[working_dir_.length() - 1] != L'\\')
    {
        working_dir_ += L"\\";
    }

    std::wstring fxdevcon;
    std::wstring driver_path;
    std::wstring inf_path;

    if (cpu_arch_ == CpuArch::x86)
    {
        if (IsWindows10())
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\x86\\";
        }
        else
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win7\\x86\\";
        }

        fxdevcon = driver_path + L"fxdevcon32.exe";
    }
    else
    {
        if (IsWindows10())
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\x64\\";
        }
        else
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win7\\x64\\";
        }

        fxdevcon = driver_path + L"fxdevcon64.exe";
    }

    std::string output;
    std::wstring cmd = fxdevcon + L" remove *DFX12";
    if (CmdExec(cmd, driver_path, output))
    {
        log = log + "fxdevcon remove DFX" + output + "\r\n";
        if (output.find("Success", 0) == std::string::npos)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

bool DfxInstall::UninstallFxSoundDriver(std::string& log)
{
    if (cpu_arch_ == CpuArch::Unknown)
    {
        return false;
    }

    if (cpu_arch_ == CpuArch::ARM64)
    {
        return UninstallARMDriver(log);
    }
    else
    {
        return UninstallIntelDriver(log);
    }
}

bool DfxInstall::CreateUpdateTask(std::string& log)
{
    if (working_dir_.empty())
    {
        return false;
    }

    if (working_dir_[working_dir_.length() - 1] != L'\\')
    {
        working_dir_ += L"\\";
    }

    std::wstring update_cmd = L"\'" + working_dir_ + L"updater.exe\' /silent\"";
    std::wstring create_task_cmd = std::wstring(L"schtasks /create /sc daily /tn FxSound\\Update /tr \"") + update_cmd + std::wstring(L" /st 10:00 /f");

    std::string output;
    if (CmdExec(create_task_cmd, working_dir_, output))
    {
        log = "Update scheduled!\r\n";
    }
    else
    {
        return false;
    }

    return true;
}

bool DfxInstall::DeleteUpdateTask(std::string& log)
{
    std::wstring delete_task_cmd = L"schtasks /delete /tn \"FxSound\\Update\" /f";

    std::string output;
    if (CmdExec(delete_task_cmd, working_dir_, output))
    {
        log = "Update schedule deleted!\r\n";
    }
    else
    {
        return false;
    }

    return true;
}

bool DfxInstall::InstallIntelDriver(std::string& log)
{
    if (cpu_arch_ == CpuArch::Unknown)
    {
        return false;
    }
    if (working_dir_.empty())
    {
        return false;
    }

    if (working_dir_[working_dir_.length() - 1] != L'\\')
    {
        working_dir_ += L"\\";
    }

    std::wstring dfxsetup;
    std::wstring apps_path = working_dir_ + APPS_FOLDER;
    std::wstring fxdevcon;
    std::wstring driver_path;
    std::wstring inf_path;
    std::string os;
    dfxsetup = apps_path + L"DfxSetupDrv.exe";

    if (cpu_arch_ == CpuArch::x86)
    {
        if (IsWindows10())
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\x86\\";
            os = "win10 x86";
        }
        else
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win7\\x86\\";
            os = "win7 x86";
        }

        fxdevcon = driver_path + L"fxdevcon32.exe";
    }
    else
    {
        if (IsWindows10())
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\x64\\";
            os = "win10 x64";
        }
        else
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win7\\x64\\";
            os = "win7 x64";
        }

        fxdevcon = driver_path + L"fxdevcon64.exe";
    }

    inf_path = driver_path + L"fxvad.inf";

    std::wstring cmd = dfxsetup + L" check";
    std::string output;
    if (CmdExec(cmd, apps_path, output))
    {
        log = "DfxSetup check " + output + "\r\n";
        if (output.find("DFX Audio Enhancer", 0) == 0)
        {
            cmd = fxdevcon + L" remove *DFX12";
            if (CmdExec(cmd, driver_path, output))
            {
                log = log + "fxdevcon remove DFX " + output + "\r\n";
                if (output.find("Success", 0) != std::string::npos)
                {
                    Sleep(1000);
                }
            }
        }
    }

    cmd = fxdevcon + L" install \"" + inf_path + L"\"";
    if (CmdExec(cmd, driver_path, output))
    {
        log = log + "fxdevcon install " + os + output + "\r\n";
        if (output.find("Success", 0) == std::string::npos)
        {
            return false;
        }

        Sleep(1000);
    }

    if (!EnableDFXDriver())
    {
        log = log + "FxSound driver is not enabled\r\n";
        return false;
    }

    std::string reg_path = std::string("Software\\DFX\\") + VENDOR_CODE + std::string("\\devices");

    cmd = dfxsetup + L" getguid";
    if (CmdExec(cmd, apps_path, output))
    {
        log = log + "DfxSetup getguid " + output + "\r\n";
        if (output.length() > 0 && output[0] == '{' && output[output.length() - 1] == '}')
        {
            HKEY h_key;
            REGSAM access_mask = KEY_ALL_ACCESS;
            if (cpu_arch_ == CpuArch::x64)
            {
                access_mask |= KEY_WOW64_64KEY;
            }
            if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, reg_path.c_str(), 0, NULL, 0, access_mask, NULL, &h_key, NULL) == ERROR_SUCCESS)
            {
                RegSetKeyValueA(h_key, NULL, "dfx_guid", REG_SZ, (LPCVOID)output.c_str(), (DWORD) output.length());
                RegCloseKey(h_key);
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    cmd = dfxsetup + L" setname";
    if (CmdExec(cmd, apps_path, output))
    {
        log = log + "DfxSetup setname " + output + "\r\n";
        if (output.find("Success", 0) == std::string::npos)
        {
            return false;
        }
    }

    /*cmd = dfxsetup + L" seticon";
    if (CmdExec(cmd, apps_path, output))
    {
        log = log + "DfxSetup seticon " + output + "\r\n";
        if (output.find("Success", 0) == std::string::npos)
        {
            return false;
        }
    }*/

    cmd = dfxsetup + L" defaultbuffersize";
    if (CmdExec(cmd, apps_path, output))
    {
        log = log + "DfxSetup defaultbuffersize " + output + "\r\n";
        if (output.find("Failed", 0) != std::string::npos)
        {
            return false;
        }

        HKEY h_key;
        REGSAM access_mask = KEY_ALL_ACCESS;
        if (cpu_arch_ == CpuArch::x64)
        {
            access_mask |= KEY_WOW64_64KEY;
        }
        if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, reg_path.c_str(), 0, NULL, 0, access_mask, NULL, &h_key, NULL) == ERROR_SUCCESS)
        {
            RegSetKeyValueA(h_key, NULL, "default_buffer_size", REG_SZ, (LPCVOID)output.c_str(), (DWORD) output.length());
            RegCloseKey(h_key);
        }
    }

    cmd = L"powercfg -REQUESTSOVERRIDE DRIVER \"FxSound Audio Enhancer\" SYSTEM";
    CmdExec(cmd, apps_path, output);

    return true;
}

bool DfxInstall::InstallARMDriver(std::string& log)
{
    std::vector<AudioDevice> audio_devices;

    if (EnumAudioOutputs(audio_devices))
    {
        for (auto audio_device : audio_devices)
        {
            if (audio_device.device_name.find(L"FxSound Audio Enhancer", 0) == 0)
            {
                log = log + "FxSound Audio Enhancer already installed\r\n";
                return true;
            }
        }
    }

    if (working_dir_[working_dir_.length() - 1] != L'\\')
    {
        working_dir_ += L"\\";
    }

    std::wstring fxdevcon;
    std::wstring driver_path;
    std::wstring inf_path;

    driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\arm64\\";
    fxdevcon = driver_path + L"fxdevcon64.exe";
    inf_path = driver_path + L"fxvad.inf";

    std::wstring cmd = fxdevcon + L" install \"" + inf_path + L"\"" + L" root\\fxvad";
    std::string output;

    if (CmdExec(cmd, driver_path, output))
    {
        log = log + "fxdevcon install ARM64 " + output + "\r\n";
        if (output.find("Success", 0) == std::string::npos)
        {
            return false;
        }

        Sleep(1000);
    }

    if (!EnableDFXDriver())
    {
        log = log + "FxSound driver is not enabled\r\n";
        return false;
    }

    std::wstring reg_path = std::wstring(L"Software\\DFX\\") + WVENDOR_CODE + std::wstring(L"\\devices");

    audio_devices.clear();
    std::wstring device_id;
    if (EnumAudioOutputs(audio_devices))
    {
        for (auto audio_device : audio_devices)
        {
            if (audio_device.device_name.find(L"FxSound Audio Enhancer", 0) == 0)
            {
                device_id = audio_device.device_guid;
                break;
            }
        }
    }

    if (device_id.length() > 0 && device_id[0] == L'{' && device_id[device_id.length() - 1] == L'}')
    {
        HKEY h_key;
        REGSAM access_mask = KEY_ALL_ACCESS;
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, reg_path.c_str(), 0, NULL, 0, access_mask, NULL, &h_key, NULL) == ERROR_SUCCESS)
        {
            RegSetKeyValue(h_key, NULL, L"dfx_guid", REG_SZ, (LPCVOID)device_id.c_str(), (DWORD) device_id.length()*sizeof(wchar_t));
            RegCloseKey(h_key);
        }
    }

    HKEY h_key;
    REGSAM access_mask = KEY_ALL_ACCESS;
    wchar_t buffer_size[] = L"40";
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, reg_path.c_str(), 0, NULL, 0, access_mask, NULL, &h_key, NULL) == ERROR_SUCCESS)
    {
        RegSetKeyValue(h_key, NULL, L"default_buffer_size", REG_SZ, (LPCVOID)buffer_size, (DWORD) wcslen(buffer_size)*sizeof(wchar_t));
        RegCloseKey(h_key);
    }

    cmd = L"powercfg -REQUESTSOVERRIDE DRIVER \"FxSound Audio Enhancer\" SYSTEM";
    CmdExec(cmd, driver_path, output);

    return true;
}

bool DfxInstall::UninstallIntelDriver(std::string& log)
{
    if (cpu_arch_ == CpuArch::Unknown)
    {
        return false;
    }
    if (working_dir_.empty())
    {
        return false;
    }

    if (working_dir_[working_dir_.length() - 1] != L'\\')
    {
        working_dir_ += L"\\";
    }

    std::wstring fxdevcon;
    std::wstring driver_path;
    std::wstring inf_path;

    if (cpu_arch_ == CpuArch::x86)
    {
        if (IsWindows10())
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\x86\\";
        }
        else
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win7\\x86\\";
        }

        fxdevcon = driver_path + L"fxdevcon32.exe";
    }
    else
    {
        if (IsWindows10())
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\x64\\";
        }
        else
        {
            driver_path = working_dir_ + DRIVERS_FOLDER + L"win7\\x64\\";
        }

        fxdevcon = driver_path + L"fxdevcon64.exe";
    }

    std::string output;
    std::wstring cmd = fxdevcon + L" remove";
    if (CmdExec(cmd, driver_path, output))
    {
        log = log + "fxdevcon remove " + output + "\r\n";
        if (output.find("Success", 0) == std::string::npos)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

bool DfxInstall::UninstallARMDriver(std::string& log)
{
    if (working_dir_[working_dir_.length() - 1] != L'\\')
    {
        working_dir_ += L"\\";
    }

    std::wstring fxdevcon;
    std::wstring driver_path;
    std::wstring inf_path;

    driver_path = working_dir_ + DRIVERS_FOLDER + L"win10\\arm64\\";
    fxdevcon = driver_path + L"fxdevcon64.exe";

    std::string output;
    std::wstring cmd = fxdevcon + L" remove root\\fxvad";
    if (CmdExec(cmd, driver_path, output))
    {
        log = log + "fxdevcon remove " + output + "\r\n";
        if (output.find("Success", 0) == std::string::npos)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return false;
    }
}

bool DfxInstall::CmdExec(const std::wstring& cmd_str, const std::wstring& working_dir, std::string& output)
{
    SECURITY_ATTRIBUTES security_attr;
    HANDLE stdout_write;
    HANDLE stdout_read;
    HANDLE stdin_write;
    HANDLE stdin_read;

    security_attr.nLength = sizeof(SECURITY_ATTRIBUTES);
    security_attr.bInheritHandle = TRUE;
    security_attr.lpSecurityDescriptor = NULL;

    if (CreatePipe(&stdout_read, &stdout_write, &security_attr, 0) == FALSE)
    {
        return false;
    }

    if (SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0) == FALSE)
    {
        return false;
    }

    if (CreatePipe(&stdin_read, &stdin_write, &security_attr, 0) == FALSE)
    {
        return false;
    }

    if (SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0) == FALSE)
    {
        return false;
    }

    STARTUPINFO startup_info = {};
    PROCESS_INFORMATION process_info = {};

    wchar_t* cmd = new wchar_t[cmd_str.length() + 1];
    lstrcpynW(cmd, cmd_str.c_str(), (int)(cmd_str.length() + 1));

    startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdOutput = stdout_write;
    startup_info.hStdError = stdout_write;
    startup_info.hStdInput = stdin_read;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
    auto ret = CreateProcess(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, working_dir.c_str(), &startup_info, &process_info);
    delete[] cmd;

    if (ret == FALSE)
    {
        return false;
    }

    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    CloseHandle(stdout_write);
    CloseHandle(stdin_read);

    output = "";
    CHAR buffer[256];
    DWORD bytes_read;

    for (;;)
    {
        ret = ReadFile(stdout_read, buffer, 255, &bytes_read, NULL);
        if (ret == FALSE || bytes_read == 0) break;
        buffer[bytes_read] = '\0';
        output += buffer;
    }

    CloseHandle(stdout_read);
    CloseHandle(stdin_write);

    return true;
}

bool DfxInstall::FindDFXDriver(const std::wstring& version)
{
    HRESULT h_res;
    IWbemLocator* wbem_loc = NULL;
    IWbemServices* wbem_svc = NULL;

    h_res = CoCreateInstance(__uuidof(WbemLocator), NULL, CLSCTX_ALL,
        __uuidof(IWbemLocator), (LPVOID *)&wbem_loc);
    if (FAILED(h_res))
    {
        return false;
    }

    h_res = wbem_loc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &wbem_svc);
    if (FAILED(h_res))
    {
        wbem_loc->Release();
        return false;
    }

    /* Set the security levels on the proxy and authenticate it to access WMI services */
    h_res = CoSetProxyBlanket(wbem_svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(h_res))
    {
        wbem_svc->Release();
        wbem_loc->Release();
        return false;
    }

    IEnumWbemClassObject* wbem_enumerator = NULL;
    h_res = wbem_svc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_PnPSignedDriver"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &wbem_enumerator);
    if (FAILED(h_res))
    {
        wbem_svc->Release();
        wbem_loc->Release();
        return false;
    }

    IWbemClassObject *wbem_obj = NULL;
    ULONG ul_return = 0;
    int param_count = 0;
    bool dfx_version_found = false;

    while (wbem_enumerator->Next(WBEM_INFINITE, 1, &wbem_obj, &ul_return) == WBEM_S_NO_ERROR)
    {
        if (ul_return == 0)
        {
            break;
        }

        VARIANT vt_prop;

        bool dfx_found = false;
        VariantInit(&vt_prop);
        h_res = wbem_obj->Get(L"HardWareID", 0, &vt_prop, 0, 0);
        if (SUCCEEDED(h_res) && (V_VT(&vt_prop) == VT_BSTR))
        {
            if (wcscmp(vt_prop.bstrVal, L"*DFX12") == 0)
            {
                dfx_found = true;
            }
        }
        VariantClear(&vt_prop);

        if (dfx_found)
        {
            VariantInit(&vt_prop);
            h_res = wbem_obj->Get(L"DriverVersion", 0, &vt_prop, 0, 0);
            if (SUCCEEDED(h_res) && (V_VT(&vt_prop) == VT_BSTR))
            {
                if (wcscmp(vt_prop.bstrVal, version.c_str()) == 0)
                {
                    dfx_version_found = true;
                }
            }
            VariantClear(&vt_prop);
        }

        wbem_obj->Release();

        if (dfx_version_found)
        {
            break;
        }
    }

    wbem_enumerator->Release();

    wbem_svc->Release();
    wbem_loc->Release();

    return dfx_version_found;
}

bool DfxInstall::IsWindows10()
{
    HRESULT h_res;
    IWbemLocator* wbem_loc = NULL;
    IWbemServices* wbem_svc = NULL;

    h_res = CoCreateInstance(__uuidof(WbemLocator), NULL, CLSCTX_ALL,
        __uuidof(IWbemLocator), (LPVOID *)&wbem_loc);
    if (FAILED(h_res))
    {
        return false;
    }

    h_res = wbem_loc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &wbem_svc);
    if (FAILED(h_res))
    {
        wbem_loc->Release();
        return false;
    }

    /* Set the security levels on the proxy and authenticate it to access WMI services */
    h_res = CoSetProxyBlanket(wbem_svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(h_res))
    {
        wbem_svc->Release();
        wbem_loc->Release();
        return false;
    }

    IEnumWbemClassObject* wbem_enumerator = NULL;
    h_res = wbem_svc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_OperatingSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL, &wbem_enumerator);
    if (FAILED(h_res))
    {
        wbem_svc->Release();
        wbem_loc->Release();
        return false;
    }

    IWbemClassObject *wbem_obj = NULL;
    ULONG ul_return = 0;
    int param_count = 0;

    while (wbem_enumerator->Next(WBEM_INFINITE, 1, &wbem_obj, &ul_return) == WBEM_S_NO_ERROR)
    {
        if (ul_return == 0)
        {
            break;
        }

        VARIANT vt_prop;

        VariantInit(&vt_prop);
        h_res = wbem_obj->Get(L"Version", 0, &vt_prop, 0, 0);
        if (SUCCEEDED(h_res) && (V_VT(&vt_prop) == VT_BSTR))
        {
            if (wcsncmp(vt_prop.bstrVal, L"10.", 3) == 0)
            {
                return true;
            }
        }
        VariantClear(&vt_prop);

        wbem_obj->Release();
    }

    wbem_enumerator->Release();

    wbem_svc->Release();
    wbem_loc->Release();

    return false;
}

bool DfxInstall::EnableDFXDriver()
{
    std::vector<AudioDevice> audio_devices;
    if (!EnumAudioOutputs(audio_devices))
    {
        return false;
    }

    for (auto audio_device : audio_devices)
    {
        if (audio_device.device_name.find(L"FxSound Audio Enhancer", 0) == 0)
        {
            if (audio_device.state & DEVICE_STATE_DISABLED)
            {
                HKEY h_reg_key;

                std::wstring reg_path = REG_PATH_DEVICES + audio_device.device_guid;
                REGSAM access_mask = KEY_ALL_ACCESS;
                if (cpu_arch_ == CpuArch::x64)
                {
                    access_mask |= KEY_WOW64_64KEY;
                }
                auto ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg_path.c_str(), 0, access_mask, &h_reg_key);
                if (ret == ERROR_SUCCESS) {
                    DWORD device_state = ENABLE_DEVICE;
                    RegSetValueEx(h_reg_key, L"DeviceState", NULL, REG_DWORD, (LPBYTE)&device_state, sizeof(DWORD));
                    RegCloseKey(h_reg_key);
                    Sleep(500);
                }
                else
                {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}

bool DfxInstall::EnumAudioOutputs(std::vector<DfxInstall::AudioDevice>& audio_devices)
{
    HRESULT h_res;
    IMMDeviceEnumerator *p_enumerator = NULL;
    IMMDeviceCollection *p_device_collection = NULL;

    h_res = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
                             CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                             (void**)&p_enumerator);
    if (FAILED(h_res))
        return false;

    h_res = p_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATEMASK_ALL & (~DEVICE_STATE_NOTPRESENT), &p_device_collection);
    if (SUCCEEDED(h_res))
    {
        UINT count;
        h_res = p_device_collection->GetCount(&count);
        for (UINT index = 0; index < count; index++)
        {
            IMMDevice *p_device = NULL;
            wchar_t* p_id;
            h_res = p_device_collection->Item(index, &p_device);
            if (SUCCEEDED(h_res))
            {
                AudioDevice audio_device;

                audio_device.device_name = GetAudioOutputName(p_device);
                if (audio_device.device_name.empty())
                {
                    p_device->Release();
                    break;
                }
                p_device->GetState(&audio_device.state);
                if (SUCCEEDED(p_device->GetId(&p_id)))
                {
                    std::wstring id = p_id;
                    auto pos = id.rfind(L'{');
                    if (pos != std::string::npos)
                    {
                        audio_device.device_guid = id.substr(pos);
                    }
                    CoTaskMemFree(p_id);
                }

                audio_devices.push_back(audio_device);
            }
            else
                break;
        }

        p_device_collection->Release();
    }

    p_enumerator->Release();

    if (SUCCEEDED(h_res))
        return true;
    else
        return false;
}

std::wstring DfxInstall::GetAudioOutputName(IMMDevice* p_device)
{
    std::wstring device_name = {};

    if (p_device == NULL)
        return device_name;

    IPropertyStore *p_prop_store = NULL;
    PROPVARIANT prop_variant;
    bool b_ret = false;

    PropVariantInit(&prop_variant);
    if (SUCCEEDED(p_device->OpenPropertyStore(STGM_READ, &p_prop_store)))
    {
        if (SUCCEEDED(p_prop_store->GetValue(PKEY_DeviceInterface_FriendlyName, &prop_variant)))
        {
            if (prop_variant.vt == VT_LPWSTR)
            {
                device_name = prop_variant.pwszVal;
            }
        }

        p_prop_store->Release();
    }
    PropVariantClear(&prop_variant);

    return device_name;
}

int cmdInstall(_In_opt_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_  LPCTSTR inf, _In_  LPCTSTR hwid)
/*++

Routine Description:

    CREATE
    Creates a root enumerated devnode and installs drivers on it

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    inf       - driver INF path
    hwid      - device id

Return Value:

    EXIT_xxxx

--*/
{
    HDEVINFO DeviceInfoSet = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    TCHAR hwIdList[LINE_LEN + 4];
    TCHAR InfPath[MAX_PATH];
    int failcode = EXIT_FAIL;

    if (Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if (inf == NULL || hwid == NULL) {
        return EXIT_USAGE;
    }
    if (!inf[0]) {
        return EXIT_USAGE;
    }

    if (!hwid[0]) {
        return EXIT_USAGE;
    }

    //
    // Inf must be a full pathname
    //
    if (GetFullPathName(inf, MAX_PATH, InfPath, NULL) >= MAX_PATH) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }

    //
    // List of hardware ID's must be double zero-terminated
    //
    ZeroMemory(hwIdList, sizeof(hwIdList));
    if (FAILED(StringCchCopy(hwIdList, LINE_LEN, hwid))) {
        goto final;
    }

    //
    // Use the INF File to extract the Class GUID.
    //
    if (!SetupDiGetINFClass(InfPath, &ClassGUID, ClassName, sizeof(ClassName) / sizeof(ClassName[0]), 0))
    {
        goto final;
    }

    //
    // Create the container for the to-be-created Device Information Element.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID, 0);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        goto final;
    }

    //
    // Now create the element.
    // Use the Class GUID and Name from the INF file.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        ClassName,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        goto final;
    }

    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)hwIdList,
        ((DWORD)_tcslen(hwIdList) + 1 + 1) * sizeof(TCHAR)))
    {
        goto final;
    }

    //
    // Transform the registry element into an actual devnode
    // in the PnP HW tree.
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
        goto final;
    }

    //
    // update the driver for the device we just created
    //
    failcode = cmdUpdate(BaseName, Machine, Flags, inf, hwid);

    final:

    if (DeviceInfoSet != INVALID_HANDLE_VALUE) {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

    return failcode;
}

int cmdUpdate(_In_opt_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ DWORD Flags, _In_  LPCTSTR inf, _In_  LPCTSTR hwid)
/*++

Routine Description:
    UPDATE
    update driver for existing device(s)

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    inf       - driver INF path
    hwid      - device id

Return Value:

    EXIT_xxxx

--*/
{
    HMODULE newdevMod = NULL;
    int failcode = EXIT_FAIL;
    UpdateDriverForPlugAndPlayDevicesProto UpdateFn;
    BOOL reboot = FALSE;
    DWORD flags = 0;
    DWORD res;
    TCHAR InfPath[MAX_PATH];

    UNREFERENCED_PARAMETER(BaseName);
    UNREFERENCED_PARAMETER(Flags);

    if (Machine) {
        //
        // must be local machine
        //
        return EXIT_USAGE;
    }
    if (inf == NULL || hwid == NULL) {
        return EXIT_USAGE;
    }
    
    if (!inf[0]) {
        return EXIT_USAGE;
    }
    if (!hwid[0]) {
        return EXIT_USAGE;
    }

    //
    // Inf must be a full pathname
    //
    res = GetFullPathName(inf, MAX_PATH, InfPath, NULL);
    if ((res >= MAX_PATH) || (res == 0)) {
        //
        // inf pathname too long
        //
        return EXIT_FAIL;
    }
    if (GetFileAttributes(InfPath) == (DWORD)(-1)) {
        //
        // inf doesn't exist
        //
        return EXIT_FAIL;
    }
    inf = InfPath;
    flags |= INSTALLFLAG_FORCE;

    //
    // make use of UpdateDriverForPlugAndPlayDevices
    //
    newdevMod = LoadLibrary(TEXT("newdev.dll"));
    if (!newdevMod) {
        goto final;
    }
    UpdateFn = (UpdateDriverForPlugAndPlayDevicesProto)GetProcAddress(newdevMod, UPDATEDRIVERFORPLUGANDPLAYDEVICES);
    if (!UpdateFn)
    {
        goto final;
    }

    if (!UpdateFn(NULL, hwid, inf, flags, &reboot)) {
        goto final;
    }

    failcode = reboot ? EXIT_REBOOT : EXIT_OK;

    final :

        if (newdevMod) {
            FreeLibrary(newdevMod);
        }

    return failcode;
}

int cmdRemove(_In_opt_ LPCTSTR BaseName, _In_opt_ LPCTSTR Machine, _In_ LPCTSTR hwid)
/*++

Routine Description:

    REMOVE
    remove devices

Arguments:

    BaseName  - name of executable
    Machine   - machine name, must be NULL
    hwid      - device id

Return Value:

    EXIT_xxxx

--*/
{
    if (!hwid) {
        //
        // arguments required
        //
        return EXIT_USAGE;
    }
    if (Machine) {
        //
        // must be local machine as we need to involve class/co installers
        //
        return EXIT_USAGE;
    }

    HDEVINFO devs = NULL;
    devs = SetupDiGetClassDevs(NULL, NULL, 0, DIGCF_ALLCLASSES);
    if (devs == INVALID_HANDLE_VALUE)
    {
        return EXIT_FAIL;
    }

    SP_DEVINFO_DATA dev_info;
    dev_info.cbSize = sizeof(SP_DEVINFO_DATA);
    WCHAR device_name[512];
    DWORD prop_type;
    bool device_removed = false;
    for (DWORD i = 0; SetupDiEnumDeviceInfo(devs, i, &dev_info); i++) {
        if (SetupDiGetDeviceRegistryProperty(devs, &dev_info, SPDRP_HARDWAREID, &prop_type, (PBYTE)device_name, sizeof(device_name), NULL))
        {

        }
        if (wcscmp(device_name, hwid) == 0) {
            if (SetupDiRemoveDevice(devs, &dev_info))
            {
                device_removed = true;
            }
            break;
        }
    }

    //Clean up
    SetupDiDestroyDeviceInfoList(devs);

    if (device_removed)
        return EXIT_OK;
    else
        return EXIT_FAIL;
}
