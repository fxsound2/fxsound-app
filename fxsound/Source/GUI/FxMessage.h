/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)
	
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
#include <functional>
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

    FxConfirmationMessage(String message, Style style, int x, int y, String yes, String no, String ok)
        : FxWindow(), message_content_(message, style, yes, no, ok)
    {
        setContent(&message_content_);
        setTopLeftPosition(x, y); // Set the desired position
        addToDesktop(0);
        setAlwaysOnTop(true);
    }
    ~FxConfirmationMessage() = default;

    FxConfirmationMessage() = delete;

    void closeButtonPressed() override
    {
        exitModalState(0);
        removeFromDesktop();
    }

    // SINGLE, COMPLETE VERSION OF showMessage
    // All parameters after 'message' have default values.
    // This eliminates ambiguity because there's only one way to call it with fewer arguments.
    static void showMessage(String message, Style style = Style::YesNo,
                            std::function<void (bool)> callback = nullptr, String yes = TRANS("Yes"), String no = TRANS("No"), String ok = TRANS("OK"), int x = -1, int y = -1)
    {
        // Construct the FxConfirmationMessage object with the provided values.
        auto* confirmation_message = new FxConfirmationMessage(message, style, x, y, yes, no, ok);

        // If x and y values are -1 (default), then center the window.
        if (x == -1 || y == -1) {
            confirmation_message->centreWithSize(confirmation_message->getWidth(), confirmation_message->getHeight());
        }

        // Only 'callback' is captured here - never confirmation_message or anything from this function's
        // stack. deleteWhenDismissed=true means confirmation_message is deleted BEFORE this runs, and
        // showMessage() has already returned by the time the user answers, so the Yes/No result is
        // encoded directly into the exitModalState() value instead (see buttonClicked() below).
        confirmation_message->enterModalState(true, ModalCallbackFunction::create([callback](int result)
        {
            if (callback)
            {
                callback(result != 0);
            }
        }), true);
    }

private:
    class MessageComponent : public Component, private Button::Listener
    {
    public:
        MessageComponent(String message, Style style, String yes, String no, String ok) : yes_button_(yes), no_button_(no), ok_button_(ok)
        {
            auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());

            message_.setText(message, NotificationType::dontSendNotification);
            message_.setFont(theme.getNormalFont());
            message_.setJustificationType(Justification::centred);
            addAndMakeVisible(message_);

            yes_button_.setMouseCursor(MouseCursor::PointingHandCursor);
            yes_button_.setWantsKeyboardFocus(true);
            addChildComponent(yes_button_);
            yes_button_.addListener(this);

            no_button_.setMouseCursor(MouseCursor::PointingHandCursor);
            addChildComponent(no_button_);
            no_button_.addListener(this);

            ok_button_.setMouseCursor(MouseCursor::PointingHandCursor);
            addChildComponent(ok_button_);
            ok_button_.addListener(this);

            auto font = theme.getTextButtonFont(ok_button_, BUTTON_HEIGHT);

            if (style == FxConfirmationMessage::Style::YesNo)
            {
                // Both buttons share one width so they look balanced, and two of them plus the gap
                // must fit inside the side margins.
                auto text_width = jmax(getTextWidth(font, yes_button_.getButtonText()),
                                       getTextWidth(font, no_button_.getButtonText()));
                button_width_ = jlimit(MIN_BUTTON_WIDTH, (WIDTH - 2 * MARGIN - BUTTON_GAP) / 2, text_width + BUTTON_TEXT_PADDING);

                yes_button_.setSize(button_width_, BUTTON_HEIGHT);
                no_button_.setSize(button_width_, BUTTON_HEIGHT);
                yes_button_.setVisible(true);
                no_button_.setVisible(true);
            }
            else
            {
                auto text_width = getTextWidth(font, ok_button_.getButtonText());
                button_width_ = jlimit(MIN_BUTTON_WIDTH, WIDTH - 2 * MARGIN, text_width + BUTTON_TEXT_PADDING);

                ok_button_.setSize(button_width_, BUTTON_HEIGHT);
                ok_button_.setVisible(true);
            }

            setSize(WIDTH, HEIGHT);
        }

        ~MessageComponent() = default;

        MessageComponent() = delete;

    private:
        static constexpr int WIDTH = 450;
        static constexpr int HEIGHT = 142;
        static constexpr int MARGIN = 20;
        static constexpr int MESSAGE_HEIGHT = (24 + 2) * 2;
        static constexpr int BUTTON_HEIGHT = 30;
        static constexpr int BUTTON_GAP = 20;
        static constexpr int MIN_BUTTON_WIDTH = 120;
        // LookAndFeel_V4::drawButtonText() insets the text by up to ~9px on each side;
        // pad a little beyond that so the label is never squeezed by drawFittedText().
        static constexpr int BUTTON_TEXT_PADDING = 24;

        static int getTextWidth(const Font& font, const String& text)
        {
#if JUCE_MAJOR_VERSION >= 8
            return roundToInt(std::ceil(GlyphArrangement::getStringWidth(font, text)));
#else
            return font.getStringWidth(text);
#endif
        }

        void resized() override
        {
            auto bounds = getLocalBounds();
            bounds.setTop(MARGIN);
            bounds.reduce(MARGIN, 0);

            RectanglePlacement placement(RectanglePlacement::xLeft
                                        | RectanglePlacement::yTop
                                        | RectanglePlacement::doNotResize);

            auto component_area = juce::Rectangle<int>(0, 0, bounds.getWidth(), MESSAGE_HEIGHT);
            message_.setBounds(placement.appliedTo(component_area, bounds));

            if (ok_button_.isVisible())
            {
                bounds.setTop(message_.getBottom() + MARGIN);

                placement = RectanglePlacement(RectanglePlacement::xMid
                                              | RectanglePlacement::yTop
                                              | RectanglePlacement::doNotResize);

                component_area = juce::Rectangle<int>(0, 0, button_width_, BUTTON_HEIGHT);
                ok_button_.setBounds(placement.appliedTo(component_area, bounds));
            }
            else
            {
                auto buttons_y = message_.getBottom() + MARGIN;

                component_area = juce::Rectangle<int>((WIDTH - (button_width_ * 2 + BUTTON_GAP)) / 2, buttons_y, button_width_, BUTTON_HEIGHT);
                yes_button_.setBounds(component_area);

                component_area = juce::Rectangle<int>(yes_button_.getRight() + BUTTON_GAP, buttons_y, button_width_, BUTTON_HEIGHT);
                no_button_.setBounds(component_area);
            }
        }

        void buttonClicked(Button* button) override
        {
            auto* parent = Component::getParentComponent();
            parent->exitModalState(button == &no_button_ ? 0 : 1); // encode the answer in the exit value itself
            parent->removeFromDesktop();
        }

        Label message_;
        TextButton yes_button_;
        TextButton no_button_;
        TextButton ok_button_;

        int button_width_ = MIN_BUTTON_WIDTH;
    };

    MessageComponent message_content_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxConfirmationMessage)
};
#endif
