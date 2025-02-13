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

#ifndef FXSYSTEMTRAYVIEW_H
#define FXSYSTEMTRAYVIEW_H

#include <JuceHeader.h>
#include "FxModel.h"
#include "FxController.h"
#include "FxNotification.h"
#include "FxSettingsDialog.h"

//==============================================================================
/*
*/

class FxSystemTrayView : public Component, public FxModel::Listener
{
public:
	FxSystemTrayView();
    ~FxSystemTrayView();

	void modelChanged(FxModel::Event model_event) override;
	void setStatus(bool power, bool processing);

private:
	static constexpr int MENU_ID_OPEN = 1;
	static constexpr int MENU_ID_POWER = 2;
	static constexpr int MENU_ID_SETTINGS = 3;
    static constexpr int MENU_ID_DONATE = 4;
	static constexpr int MENU_ID_EXIT = 5;
	static constexpr int PRESET_MENU_ID_START = 101;
	static constexpr int OUTPUT_MENU_ID_START = 201;
	static constexpr int WMAPP_FXTRAYICON = WM_APP + 1;
	static const GUID trayIconGuid_;

	static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	void showContextMenu();
	void showNotification();	

	bool system_notifications_available_;
	FxNotification notification_;
	WNDPROC componentWndProc_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxSystemTrayView)
};

#endif