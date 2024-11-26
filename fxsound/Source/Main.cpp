/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <JuceHeader.h>
#include <comdef.h>
#include "GUI/FxSystemTrayView.h"
#include "GUI/FxController.h"
#include "GUI/FxTheme.h"
#include "GUI/FxMainWindow.h"
#include "AudioPassthru.h"
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

//==============================================================================
class FxSoundApplication : public JUCEApplication
{
public:
    //==============================================================================
    FxSoundApplication() { }

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return false; }

    //==============================================================================
    void initialise (const String& commandline) override
    {
        // This method is where you should put your application's initialisation code..
        try
        {
            SetUnhandledExceptionFilter(unhandledExceptionFilter);

            HRESULT hRes = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (SUCCEEDED(hRes))
            {
                CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
            }
            
            setWorkingDirectory();

            LookAndFeel::setDefaultLookAndFeel(&theme_);

            FxController::getInstance().config(commandline);

            audio_passthru_ = std::make_unique<AudioPassthru>();
            main_window_ = std::make_unique<FxMainWindow>();
            system_tray_view_.reset(new FxSystemTrayView());

            FxController::getInstance().init(main_window_.get(), system_tray_view_.get(), audio_passthru_.get());
        }
        catch (const std::exception& e)
        {
            juce::NativeMessageBox::showMessageBox(juce::AlertWindow::AlertIconType::WarningIcon, getApplicationName(), e.what());
            quit();
        }
    }

    void suspended() override
    {
        FxController::getInstance().stopTimer();
    }

    void resumed() override
    {
        FxController::getInstance().startTimer(100);
    }

    void shutdown() override
    {
        if (main_window_.get() != nullptr)
        {
            // Add your application's shutdown code here..

            audio_passthru_.reset();

            system_tray_view_.reset();

            main_window_.reset(); // (deletes our window)
        }

        LookAndFeel::setDefaultLookAndFeel(nullptr);

        CoUninitialize();
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        
        quit();
    }

    void anotherInstanceStarted (const String&) override
    {
        FxController::getInstance().showMainWindow();
    }

private:
    FxTheme theme_;
    static constexpr int MAX_FRAMES = 64;

    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exception_info)
    {
        String message = String::formatted("Unhandled exception\nException code: 0x%X\nException flags: 0x%X\nException address: 0x%p\n",
            exception_info->ExceptionRecord->ExceptionCode,
            exception_info->ExceptionRecord->ExceptionFlags,
            exception_info->ExceptionRecord->ExceptionAddress);

        auto& controller = FxController::getInstance();
        controller.logMessage("\n"+message);

        void* stack[MAX_FRAMES];
        HANDLE process = GetCurrentProcess();
        String stacktrace_info;

        WORD frames = CaptureStackBackTrace(0, MAX_FRAMES, stack, nullptr);

        SymInitialize(process, nullptr, TRUE);

        for (WORD i = 0; i < frames; i++)
        {
            DWORD64 address = (DWORD64)(stack[i]);

            char symbol_buffer[sizeof(SYMBOL_INFO) + 256] = { 0 };
            SYMBOL_INFO* symbol = (SYMBOL_INFO*)symbol_buffer;
            symbol->MaxNameLen = 255;
            symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

            if (SymFromAddr(process, address, nullptr, symbol))
            {
                DWORD lineDisplacement;
                IMAGEHLP_LINE64 lineInfo = { 0 };
                lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

                stacktrace_info += String::formatted("%S at 0x%llX\n", symbol->Name, symbol->Address);

                if (SymGetLineFromAddr64(process, address, &lineDisplacement, &lineInfo))
                {
                    char* path = std::strrchr(lineInfo.FileName, '\\');
                    char* file_name;
                    if (path != nullptr)
                    {
                        file_name = path + 1;
                    }
                    else
                    {
                        file_name = lineInfo.FileName;
                    }

                    stacktrace_info += String::formatted("    File: %S, Line: %d\n", file_name, lineInfo.LineNumber);
                }
            }
        }

        SymCleanup(process);

        controller.logMessage(stacktrace_info);

        MessageBox(NULL, message.toWideCharPointer(), L"FxSound", MB_ICONERROR | MB_OK);

        return EXCEPTION_EXECUTE_HANDLER;
    }

    void setWorkingDirectory()
    {
        wchar_t module_file[MAX_PATH];
        DWORD length = GetModuleFileName(NULL, module_file, MAX_PATH);
        if (length > 0)
        {
            std::wstring path(module_file);
            size_t pos = path.find_last_of(L"\\/");
            if (pos != std::string::npos)
            {
                SetCurrentDirectory(path.substr(0, pos).c_str());
            }
        }
    };

    std::unique_ptr<FxMainWindow> main_window_;
    
    std::unique_ptr<FxSystemTrayView> system_tray_view_;
    std::unique_ptr<AudioPassthru> audio_passthru_;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (FxSoundApplication)
