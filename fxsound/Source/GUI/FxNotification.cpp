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

#include "FxNotification.h"

FxNotification::FxNotification()
{
	setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());

	icon_ = Drawable::createFromImageData(BinaryData::logowhite_png, BinaryData::logowhite_pngSize);
	icon_->setTransformToFit(juce::Rectangle<float>(15.0f, 10.0f, (float)ICON_WIDTH, (float)ICON_HEIGHT),
								{ RectanglePlacement::xMid | RectanglePlacement::yMid });
	addAndMakeVisible(icon_.get());

    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
	for (int i=0; i<3; i++)
	{
		message_lines_[i].setColour(Label::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::defaultText));
		message_lines_[i].setJustificationType(Justification::centred);
		message_lines_[i].setBorderSize(BorderSize<int>(1, 0, 2, 0));
		message_lines_[i].setMinimumHorizontalScale(1.0);
		addChildComponent(message_lines_[i]);
	}
	
	message_link_.setJustificationType(Justification::centredLeft);
	addChildComponent(message_link_);

    setSize(WIDTH, HEIGHT);

	setVisible(false);

    setAlwaysOnTop(true);
}

FxNotification::~FxNotification()
{
	removeFromDesktop();

	setLookAndFeel(nullptr);

	if (isTimerRunning())
	{
		stopTimer();
	}
}

void FxNotification::setMessage(const String& message, const std::pair<String, String>& link, bool autohide)
{
    setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());

	if (isTimerRunning())
	{
		if (getTimerInterval() == 7000)
		{
			stopTimer();
		}
		else
		{
			return;
		}
	}

	message_link_.setButtonText(link.first);
	message_link_.setURL(URL(link.second));

    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
    auto font = theme.getSmallFont().withHeight(17.0f);

	for (int i=0; i<3; i++)
	{
		message_lines_[i].setVisible(false);
		message_lines_[i].setText("", NotificationType::dontSendNotification);
        message_lines_[i].setFont(font);
	}

	StringArray lines;
	lines.addLines(message);


	int width;
	auto line_width = 0;
	auto line_count = 0;
	auto link_width = 0;
	link_line_ = 0;

    width = WIDTH;

	for (auto i=0; i<lines.size(); i++)
	{
		if (i == 3)
		{
			break;
		}
		line_count++;

		message_lines_[i].setText(lines[i], NotificationType::dontSendNotification);

		if (message_link_.getButtonText().isNotEmpty())
		{
			message_lines_[i].setJustificationType(Justification::centredLeft);
			if (i == lines.size()-1)
			{
				link_width = message_link_.getTextWidth();
				link_line_ = i;
			}
		}
		else
		{
			message_lines_[i].setJustificationType(Justification::centred);
		}

        auto margin = autohide ? 80 : 40;
		line_width = font.getStringWidth(lines[i]) + link_width;
		if (line_width > WIDTH-margin)
		{
			if (line_width > MAX_WIDTH-margin)
			{
				width = MAX_WIDTH;
				if (link_width > 0)
				{
					link_line_ = i+1;
				}
			}
			else
			{
                width = line_width + margin;				
			}
		}
	}

    setSize(width, line_count * 20 + 60);
}

void FxNotification::showMessage(bool autohide)
{
	int last_line = 0;
    int y = 0;

    auto x = autohide ? 40 : 20;
	for (auto i=0; i<3 && message_lines_[i].getText().isNotEmpty(); i++)
	{
        message_lines_[i].setBounds(x, i * 20 + 30, getWidth() - (x*2), 20);
		message_lines_[i].setVisible(true);
		last_line = i;
	}

	if (message_link_.getButtonText().isNotEmpty())
	{
		auto width = message_link_.getTextWidth();		
		if (last_line == link_line_)
		{
            auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
			auto font = theme.getSmallFont().withHeight(17.0f);
			x += font.getStringWidth(message_lines_[last_line].getText());
		}
        message_link_.setBounds(x, link_line_ * 20 + 30, width, 20);
		message_link_.setVisible(true);

        y = message_link_.getBottom() + 10;
	}
    else
    {
        y = message_lines_[last_line].getBottom() + 10;
    }

    if (autohide)
    {
        addToDesktop(0);
        toFront(true);
        Desktop::getInstance().getAnimator().fadeIn(this, 200);

        if (message_link_.getButtonText().isEmpty() && message_link_.getURL().isEmpty())
        {
            startTimer(7000);
        }
        else
        {
            startTimer(8000);
        }
    }
    else
    {
        icon_->setVisible(false);
        Desktop::getInstance().getAnimator().fadeIn(this, 200);
        setVisible(true);
    }
}

void FxNotification::paint(Graphics& g)
{
    g.setFillType(FillType(Colour(0x0).withAlpha(1.0f)));
        
    g.fillRoundedRectangle(0, 0, (float)getWidth(), (float)getHeight(), 16);

    DropShadow shadow;
    Path path;
    path.addRoundedRectangle(0, 0, (float)getWidth(), (float)getHeight(), 16);
    shadow.radius = 5;
    shadow.drawForPath(g, path);
}

void FxNotification::timerCallback()
{
	stopTimer();
	setVisible(false);
	removeFromDesktop();
}