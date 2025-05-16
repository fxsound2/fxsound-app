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

#ifndef FXNOTIFICATION_H
#define FXNOTIFICATION_H

#include <JuceHeader.h>
#include "FxTheme.h"
#include "FxHyperlink.h"
#include "FxController.h"

class FxNotification : public Component, private Timer
{
public:
	FxNotification();
	~FxNotification();

    static constexpr int WIDTH = 216;
    static constexpr int HEIGHT = 80;
    static constexpr int MAX_WIDTH = 560;
    static constexpr int MAX_HEIGHT = 120;

	void setMessage(const String& message, const std::pair<String, String>& link, bool autohide=true);
	void showMessage(bool autohide=true);

private:	
	static constexpr int ICON_WIDTH = 79;
	static constexpr int ICON_HEIGHT = 12;
    static constexpr int AD_WIDTH = 216;
    static constexpr int AD_HEIGHT = 36;
    static constexpr int TITLE_HEIGHT = 24;
    static constexpr int HYPERLINK_HEIGHT = 24;

	void paint(Graphics&) override;

	void timerCallback() override;

	std::unique_ptr<Drawable> icon_;
	Label message_lines_[3];
	FxHyperlink message_link_;

	int line_count_;
	int link_line_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxNotification)
};

#endif