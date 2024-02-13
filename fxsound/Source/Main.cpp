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
            HRESULT hRes = CoInitializeEx(0, COINIT_MULTITHREADED);
            if (SUCCEEDED(hRes))
            {
                CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                    RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
            }
            
            LookAndFeel::setDefaultLookAndFeel(&theme_);

            FxController::getInstance().config(commandline);

            audio_passthru_ = std::make_unique<AudioPassthru>();
            main_window_ = std::make_unique<FxMainWindow>();
            system_tray_view_.reset(new FxSystemTrayView());

            FxController::getInstance().init(main_window_.get(), audio_passthru_.get());
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

    std::unique_ptr<FxMainWindow> main_window_;
    
    std::unique_ptr<FxSystemTrayView> system_tray_view_;
    std::unique_ptr<AudioPassthru> audio_passthru_;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (FxSoundApplication)
