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

#ifndef FXVIEW_H
#define FXVIEW_H

#include <JuceHeader.h>
#include "FxModel.h"
#include "FxController.h"
#include "FxNotification.h"
#include "FxComboBox.h"
#include "FxHyperlink.h"

//==============================================================================
/*
*/
class FxView : public Component, public ComboBox::Listener, public FxModel::Listener
{
public:
	FxView();
	~FxView() = default;

protected:
	static constexpr int LIST_WIDTH = 225;
	static constexpr int LIST_HEIGHT = 50;
	
    void showErrorNotification(bool show);

	void comboBoxChanged(ComboBox* combobox) override;
	void modelChanged(FxModel::Event model_event) override;

	void mouseEnter(const MouseEvent&) override;
	void mouseExit(const MouseEvent&) override;

	FxComboBox preset_list_;
	FxComboBox endpoint_list_;
    FxNotification error_notification_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxView)
};

#endif