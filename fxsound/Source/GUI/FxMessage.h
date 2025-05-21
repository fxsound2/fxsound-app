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

#ifndef FXMESSAGE_H
#define FXMESSAGE_H

#include <JuceHeader.h>
#include "FxWindow.h"
#include "FxHyperlink.h"
#include "FxTheme.h"

//==============================================================================
/*
*/
class FxMessage : public FxWindow
{
public:
	FxMessage(String message, const std::pair<String, String>& link);
	~FxMessage() = default;

    bool keyPressed(const KeyPress& key) override;

	void closeButtonPressed() override;


    static void showMessage(String message, const std::pair<String, String>& link);

private:
	class MessageComponent : public Component
	{
	public:
		MessageComponent(String message, const std::pair<String, String>& link);
		~MessageComponent() = default;

		void resized() override;

	private:
		static constexpr int WIDTH = 400;
		static constexpr int HEIGHT = 80;
		static constexpr int MESSAGE_HEIGHT = (24 + 2);
		static constexpr int HYPERLINK_HEIGHT = 24;

		Label message_;
		FxHyperlink link_;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MessageComponent)
	};

	MessageComponent message_content_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxMessage)
};

class FxConfirmationMessage : public FxWindow
{
public:
    enum Style { YesNo = 1, OK };

    FxConfirmationMessage(String message, Style style = Style::YesNo) : FxWindow(), message_content_(message, style)
    {
        setContent(&message_content_);
        centreWithSize(getWidth(), getHeight());
        addToDesktop(0);
        setAlwaysOnTop(true);
       
        style_ = style;
    }
    ~FxConfirmationMessage() = default;

    FxConfirmationMessage() = delete;

    void closeButtonPressed() override
    {
        exitModalState(0);
        removeFromDesktop();
    }

    static bool showMessage(String message, Style style = Style::YesNo)
    {
        FxConfirmationMessage confirmation_message(message, style);
        confirmation_message.runModalLoop();
        if (confirmation_message.style_ == Style::YesNo)
        {
            if (confirmation_message.message_content_.isYesClicked())
            {
                return true;
            }

            return false;
        }
        else
        {
            return true;
        }
    }

private:
    class MessageComponent : public Component, private Button::Listener
    {
    public:
        MessageComponent(String message, Style style) : yes_button_(TRANS("Yes")), no_button_(TRANS("No")), ok_button_(TRANS("OK"))
        {
            auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

            yes_clicked_ = false;

            message_.setText(message, NotificationType::dontSendNotification);
            message_.setFont(theme.getNormalFont());
            message_.setJustificationType(Justification::centred);
            addAndMakeVisible(message_);

            yes_button_.setMouseCursor(MouseCursor::PointingHandCursor);
            yes_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
            yes_button_.setWantsKeyboardFocus(true);
            addChildComponent(yes_button_);
            yes_button_.addListener(this);

            no_button_.setMouseCursor(MouseCursor::PointingHandCursor);
            no_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
            addChildComponent(no_button_);
            no_button_.addListener(this);

            ok_button_.setMouseCursor(MouseCursor::PointingHandCursor);
            ok_button_.setSize(BUTTON_WIDTH, BUTTON_HEIGHT);
            addChildComponent(ok_button_);
            ok_button_.addListener(this);

            if (style == FxConfirmationMessage::Style::YesNo)
            {
                yes_button_.setVisible(true);
                no_button_.setVisible(true);
            }
            else
            {
                ok_button_.setVisible(true);
            }

            setSize(WIDTH, HEIGHT);
        }
        ~MessageComponent() = default;

        MessageComponent() = delete;

        bool isYesClicked()
        {
            return yes_clicked_;
        }

    private:
        static constexpr int WIDTH = 450;
        static constexpr int HEIGHT = 142;
        static constexpr int MESSAGE_HEIGHT = (24 + 2) * 2;
        static constexpr int BUTTON_WIDTH = 120;
        static constexpr int BUTTON_HEIGHT = 30;

        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.setTop(20);
            bounds.reduce(20, 0);

            RectanglePlacement placement(RectanglePlacement::xLeft
                                        | RectanglePlacement::yTop
                                        | RectanglePlacement::doNotResize);

            auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), MESSAGE_HEIGHT);
            message_.setBounds(placement.appliedTo(component_area, bounds));

            if (ok_button_.isVisible())
            {
                bounds.setTop(message_.getBottom() + 20);

                placement = RectanglePlacement(RectanglePlacement::xMid
                                              | RectanglePlacement::yTop
                                              | RectanglePlacement::doNotResize);

                component_area = juce::Rectangle<int>(0, 0, BUTTON_WIDTH, BUTTON_HEIGHT);
                ok_button_.setBounds(placement.appliedTo(component_area, bounds));
            }
            else
            {
                component_area = juce::Rectangle<int>((WIDTH - ((BUTTON_WIDTH * 2) + 20)) / 2, message_.getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
                yes_button_.setBounds(component_area);

                component_area = juce::Rectangle<int>(yes_button_.getRight() + 20, message_.getBottom() + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
                no_button_.setBounds(component_area);
            }
        }

        void buttonClicked(Button* button) override
        {
            if (button == &yes_button_)
            {
                yes_clicked_ = true;
            }

            Component::getParentComponent()->exitModalState(0);
            Component::getParentComponent()->removeFromDesktop();
        }

        Label message_;
        TextButton yes_button_;
        TextButton no_button_;
        TextButton ok_button_;

        bool yes_clicked_;
    };

    MessageComponent message_content_;

    Style style_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxConfirmationMessage)
};
#endif
