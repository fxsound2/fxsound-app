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

#include <JuceHeader.h>
#include "FxWindow.h"

//==============================================================================
FxWindow::FxWindow(String name) : title_bar_(name), content_(nullptr)
{
    setLookAndFeel(&LookAndFeel::getDefaultLookAndFeel());

	setSize(64, 64);

	title_bar_.setSize(64, FxTheme::TITLE_BAR_HEIGHT - 1);
	addAndMakeVisible(title_bar_);

    draw_shadow_ = true;
    shadow_width_ = SHADOW_WIDTH;
}

FxWindow::~FxWindow()
{
	setLookAndFeel(nullptr);
}

void FxWindow::setContent(Component* content)
{
	int x = -1;
	int y = -1;

	if (content_ != nullptr && content_ != content)
	{
		if (content->getWidth() > content_->getWidth())
		{
			x = getX() - (content->getWidth() - content_->getWidth()) / 2;
		}
		else
		{
			x = getX() + (content_->getWidth() - content->getWidth()) / 2;
		}

		if (content->getHeight() > content_->getHeight())
		{
			y = getY() - (content->getHeight() - content_->getHeight()) / 2;
		}
		else
		{
			y = getY() + (content_->getHeight() - content->getHeight()) / 2;
		}
	}

	if (content != content_)
	{
		removeChildComponent(content_);
		content_ = content;
		addAndMakeVisible(content);
	}

	content_->setBounds(shadow_width_, title_bar_.getBottom() + 1, content_->getWidth(), content_->getHeight());
	if (x >= 0 && y >= 0)
	{
		setBounds(x-shadow_width_, y-shadow_width_, content_->getWidth() + shadow_width_*2, content_->getHeight() + title_bar_.getHeight() + FxTheme::WINDOW_CORNER_RADIUS + shadow_width_*2);
	}
	else
	{
		setSize(content_->getWidth() + shadow_width_*2, content_->getHeight() + title_bar_.getHeight() + FxTheme::WINDOW_CORNER_RADIUS + shadow_width_*2);
	}
}

void FxWindow::startLogoAnimation()
{
	title_bar_.startLogoAnimation();
}

void FxWindow::stopLogoAnimation()
{
	title_bar_.stopLogoAnimation();
}

void FxWindow::addToolbarButton(Button* toolbarButton)
{
	title_bar_.addToolbarButton(toolbarButton);
}

void FxWindow::enableShadow(bool enable)
{
    draw_shadow_ = enable;
    if (enable)
    {
        shadow_width_ = SHADOW_WIDTH;
    }
    else
    {
        shadow_width_ = 0;
    }
}

bool FxWindow::isShadowEnabled()
{
    return draw_shadow_;
}

void FxWindow::paint(Graphics& g)
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());

    if (draw_shadow_ && shadow_width_ != 0)
    {
        DropShadow shadow;
        Path path;
        path.addRoundedRectangle(juce::Rectangle<int>(shadow_width_, shadow_width_, getWidth() - shadow_width_ * 2, getHeight() - shadow_width_ * 2), (float)FxTheme::WINDOW_CORNER_RADIUS);
        shadow.radius = shadow_width_;
        shadow.drawForPath(g, path);
    }
    
	g.setFillType(FillType(theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground)));
    if (isOpaque())
    {       
        g.fillRect((float)shadow_width_, (float)shadow_width_, (float)getWidth() - shadow_width_*2, (float)getHeight() - shadow_width_*2);
    }
    else
    {
        g.fillRoundedRectangle((float)shadow_width_, (float)shadow_width_, (float)getWidth() - shadow_width_ * 2, (float)getHeight() - shadow_width_ * 2, (float)FxTheme::WINDOW_CORNER_RADIUS);        
    }

	g.setColour(Colour(0x2b2b2b).withAlpha(1.0f));
	g.drawLine((float)shadow_width_, (float)title_bar_.getBottom(), (float)getWidth() - shadow_width_*2, (float)title_bar_.getBottom());
}

void FxWindow::resized()
{
	title_bar_.setBounds(FxTheme::WINDOW_CORNER_RADIUS + shadow_width_, shadow_width_, getWidth() - FxTheme::WINDOW_CORNER_RADIUS*2 - shadow_width_*2, title_bar_.getHeight());
	if (content_ != nullptr)
	{
		content_->setBounds(shadow_width_, title_bar_.getBottom() + 1, content_->getWidth(), content_->getHeight());
	}
}

void FxWindow::CloseButton::paintButton(Graphics& g, bool, bool)
{
	auto& theme = dynamic_cast<LookAndFeel_V4&>(getLookAndFeel());
	g.fillAll(theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground));

	auto rect = Justification(Justification::centred)
		.appliedToRectangle(Rectangle<int>(getHeight(), getHeight()), getLocalBounds())
		.toFloat();

	Path shape;
	shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, 0.08f);
	shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, 0.08f);

	g.setColour(Colour(0xffe63462));
	g.fillPath(shape, shape.getTransformToScaleToFit(rect, true));
}

FxWindow::TitleBar::TitleBar(String name)
{
	name_ = name;
	dragging_ = false;

	addChildComponent(title_);
	if (name_.isEmpty())
	{
		icon_ = Drawable::createFromImageData(BinaryData::logowhite_png, BinaryData::logowhite_pngSize);
		icon_->setTransformToFit(juce::Rectangle<float>(0, (float)(FxTheme::TITLE_BAR_HEIGHT - ICON_HEIGHT) / 2, (float)ICON_WIDTH, (float)ICON_HEIGHT),
			{ RectanglePlacement::xLeft | RectanglePlacement::yMid });

		animation_icon_ = Drawable::createFromImageData(BinaryData::logored_png, BinaryData::logored_pngSize);
		animation_icon_->setTransformToFit(juce::Rectangle<float>(0, (float)(FxTheme::TITLE_BAR_HEIGHT - ICON_HEIGHT) / 2, (float)ICON_WIDTH, (float)ICON_HEIGHT),
			{ RectanglePlacement::xLeft | RectanglePlacement::yMid });
		animation_icon_->setAlpha(0.0f);
	}
	else
	{
		icon_ = Drawable::createFromImageData(BinaryData::FxSound_White_Bars_svg, BinaryData::FxSound_White_Bars_svgSize);
		auto width = (ICON_HEIGHT - 1)*icon_->getWidth() / icon_->getHeight();
		icon_->setTransformToFit(juce::Rectangle<float>(0, (float)(FxTheme::TITLE_BAR_HEIGHT - ICON_HEIGHT - 1) / 2, (float)width, (float)ICON_HEIGHT - 1),
			{ RectanglePlacement::xLeft | RectanglePlacement::yMid });

		auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
		auto font = theme.getNormalFont();

		title_.setText(name_, NotificationType::dontSendNotification);
		title_.setColour(Label::ColourIds::textColourId, theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::highlightedText));
		title_.setFont(font);
		title_.setJustificationType(Justification::centredLeft);
		title_.setSize(font.getStringWidth(name_) * 2, (int)font.getHeight());
		title_.setVisible(true);
	}

	addAndMakeVisible(icon_.get());
	if (animation_icon_.get() != nullptr)
	{
		addAndMakeVisible(animation_icon_.get());
	}
	
	setFocusContainer(true);
	setFocusContainerType(FocusContainerType::keyboardFocusContainer);

	close_button_.setMouseCursor(MouseCursor::PointingHandCursor);
	close_button_.setSize(CLOSE_BUTTON_WIDTH, CLOSE_BUTTON_WIDTH);
	addAndMakeVisible(close_button_);
	close_button_.addListener(this);
}

void FxWindow::TitleBar::startLogoAnimation()
{
	if (animation_icon_.get() != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(icon_.get(), 600);
		Desktop::getInstance().getAnimator().fadeIn(animation_icon_.get(), 600);
	}
}
void FxWindow::TitleBar::stopLogoAnimation()
{
	if (animation_icon_.get() != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(animation_icon_.get(), 600);
		Desktop::getInstance().getAnimator().fadeIn(icon_.get(), 600);
	}
}

void FxWindow::TitleBar::addToolbarButton(Button* toolbarButton)
{
	if (toolbarButton != nullptr)
	{
		// If translated text is added to the text button in the toolbar, it doesn't change on language change
		// So, the text button is added to the toolbar with button name in English and then button text is translated from button name
		// On repaint, the translated button text is updated
		if (TextButton* text_button = dynamic_cast<TextButton*>(toolbarButton))
		{
			auto& text = text_button->getName();
			if (text.isNotEmpty())
			{
				text_button->setButtonText(TRANS(text));
			}
		}

		if (!getWantsKeyboardFocus())
		{
			setWantsKeyboardFocus(true);
		}

		toolbar_buttons_.push_back(toolbarButton);
		addAndMakeVisible(toolbarButton);
	}
}

void FxWindow::TitleBar::paint(Graphics& g)
{
	auto& theme = dynamic_cast<FxTheme&>(getLookAndFeel());
	g.fillAll(theme.getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::windowBackground));

    title_.setText(TRANS(name_), NotificationType::dontSendNotification);
    auto font = theme.getNormalFont();
    title_.setFont(font);
    title_.setSize(font.getStringWidth(TRANS(name_)) * 2, (int)font.getHeight());

	for (auto* button : toolbar_buttons_)
	{
		if (TextButton* text_button = dynamic_cast<TextButton*>(button))
		{
			auto& text = text_button->getName();
			if (text.isNotEmpty())
			{
				text_button->setButtonText(TRANS(text));
			}
		}
	}
}

void FxWindow::TitleBar::resized()
{
	RectanglePlacement placement = { RectanglePlacement::xRight | RectanglePlacement::yMid | RectanglePlacement::doNotResize };
	close_button_.setBounds(placement.appliedTo(close_button_.getLocalBounds(), getLocalBounds()));
	
	if (!title_.isVisible())
	{
		auto bounds = juce::Rectangle<int>(0, close_button_.getY(), ICON_WIDTH, ICON_HEIGHT);
		icon_->setBounds(bounds);
		if (animation_icon_.get() != nullptr)
		{
			animation_icon_->setBounds(bounds);
		}
	}
	else
	{
		auto width = (ICON_HEIGHT - 1)*icon_->getWidth() / icon_->getHeight();
		auto bounds = juce::Rectangle<int>(0, close_button_.getY(), width, ICON_HEIGHT - 1);
		icon_->setBounds(bounds);

		placement = { RectanglePlacement::xLeft | RectanglePlacement::yMid | RectanglePlacement::doNotResize };
		title_.setBounds(placement.appliedTo(title_.getLocalBounds(), getLocalBounds()).withX(width + 2));
	}

	if (!toolbar_buttons_.empty())
	{
		int x = close_button_.getWidth() + 20;
		for (auto button : toolbar_buttons_)
		{
			auto destBounds = getLocalBounds().reduced(x, 0);
			button->setBounds(placement.appliedTo(button->getLocalBounds(), destBounds));
			x += button->getWidth() + 20;
		}
	}
}

void FxWindow::TitleBar::buttonClicked(Button* button)
{
	if (button == &close_button_)
	{
		auto parent = dynamic_cast<FxWindow*>(getParentComponent());
		parent->closeButtonPressed();
	}
}

void FxWindow::TitleBar::mouseDown(const MouseEvent& e)
{
	dragging_ = true;
	auto parent = dynamic_cast<FxWindow*>(getParentComponent());
	dragger_.startDraggingComponent(parent, e);
}

void FxWindow::TitleBar::mouseDrag(const MouseEvent& e)
{
	if (dragging_)
	{
		auto parent = dynamic_cast<FxWindow*>(getParentComponent());
		dragger_.dragComponent(parent, e, nullptr);
	}
}

void FxWindow::TitleBar::mouseUp(const MouseEvent&)
{
	dragging_ = false;
}