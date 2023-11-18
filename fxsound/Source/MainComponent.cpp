/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(const String& name) : Component(name)
{
	colour_scheme_ = 0;
	fx_view_ = MinView;
	resize_view_ = true;

	min_view_ = std::make_unique<FxMinView>(this);
	addAndMakeVisible(min_view_.get());
	min_view_->setResizeViewListener(this);

	expanded_view_ = std::make_unique<FxExpandedView>(this);
	addChildComponent(expanded_view_.get());
	expanded_view_->setResizeViewListener(this);

	look_and_feel_ = std::make_unique<UITheme>();
	setLookAndFeel(look_and_feel_.get());

	setSize(min_view_->getWidth(), min_view_->getHeight());

	announcement_ = std::make_unique<AnnouncementComponent>();

	show_caption_ = settings_.getBool("ShowCaption");
	if (show_caption_)
	{
		caption_ = settings_.getString("Caption");
		font_size_ = settings_.getDouble("FontSize");
		font_color_ = settings_.getInt("FontColor");
	}
}

MainComponent::~MainComponent()
{
	setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::paint(Graphics& g)
{
	if (animator_.isAnimating(this))
	{
		auto bounds = getLocalBounds();
		bounds.reduce(20, 20);

		DropShadow shadow;
		Path path;
		path.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 20);
		shadow.offset = Point<int>(10, 10);
		shadow.drawForPath(g, path);

		g.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
		g.fillPath(path);
	}
}

void MainComponent::resized()
{
	if (resize_view_)
	{
		if (fx_view_ == MinView)
		{
			if (getWidth() == min_view_->getWidth() &&
				getHeight() == min_view_->getHeight())
			{
				min_view_->show();
				resize_view_ = false;
			}
		}
		else
		{
			if (getWidth() == expanded_view_->getWidth() &&
				getHeight() == expanded_view_->getHeight())
			{
				expanded_view_->show();
				resize_view_ = false;
			}
		}
	}
}

void MainComponent::mouseDown(const MouseEvent& e)
{
	dragger_.startDraggingComponent(this, e);
}

void MainComponent::mouseDrag(const MouseEvent& e)
{
	dragger_.dragComponent(this, e, nullptr);
}

void MainComponent::mouseDoubleClick(const MouseEvent& e)
{
	if (++colour_scheme_ > 3)
		colour_scheme_ = 0;
	switch (colour_scheme_)
	{
	case 0:
		look_and_feel_->setColourScheme(LookAndFeel_V4::getDarkColourScheme());
		repaint();
		break;

	case 1:
		look_and_feel_->setColourScheme(LookAndFeel_V4::getMidnightColourScheme());
		repaint();
		break;

	case 2:
		look_and_feel_->setColourScheme(LookAndFeel_V4::getGreyColourScheme());
		repaint();
		break;

	case 3:
		look_and_feel_->setColourScheme(LookAndFeel_V4::getLightColourScheme());
		repaint();
	}
}

bool MainComponent::hitTest(int x, int y)
{
	if (fx_view_ == MinView)
	{
		return min_view_->hitTest(x, y);
	}
	else
	{
		return expanded_view_->hitTest(x, y);
	}
}

void MainComponent::userTriedToCloseWindow()
{
	JUCEApplication::getInstance()->systemRequestedQuit();
}

void MainComponent::resizeView()
{
	int x, y, width, height;

	x = getX();
	y = getY();

	auto desktop_area = Desktop::getInstance().getDisplays().getMainDisplay().userArea;

	if (min_view_->isVisible())
	{
		min_view_->setVisible(false);

		width = expanded_view_->getWidth();
		height = expanded_view_->getHeight();

		x = x - (width - min_view_->getWidth()) / 2;
		y = y - (height - min_view_->getHeight()) / 2;

		fx_view_ = ExpandedView;
	}
	else
	{
		expanded_view_->setVisible(false);

		width = min_view_->getWidth();
		height = min_view_->getHeight();

		x = x + (expanded_view_->getWidth() - width) / 2;

		y = y + (expanded_view_->getHeight() - height) / 2;

		fx_view_ = MinView;
	}

	if (x + width + 20 > desktop_area.getWidth())
	{
		x = desktop_area.getWidth() - (width + 20);
	}
	if (y + height + 0 > desktop_area.getHeight())
	{
		y = desktop_area.getHeight() - (height + 20);
	}

	Rectangle<int> final_bounds(x, y, width, height);
	animator_.animateComponent(this, final_bounds, 1.0, 2000, false, 0, 0);
	resize_view_ = true;
}

void MainComponent::showAnnouncement()
{
	WebInputStream stream(URL(CHECK_ANNOUNCEMENT_URL), false);
	var attribs = JSON::parse(stream);
	String url = attribs[ATTRIB_URL];
	int width = attribs[ATTRIB_WIDTH];
	int height = attribs[ATTRIB_HEIGHT];

	if (url.isNotEmpty())
	{
		settings_.setString("URL", url);
		settings_.setInt("width", width);
		settings_.setInt("height", height);

		juce::Rectangle<int> area(0, 0, width, height);
		RectanglePlacement placement(RectanglePlacement::xRight
			| RectanglePlacement::yBottom
			| RectanglePlacement::doNotResize);
		auto bounds = placement.appliedTo(area, Desktop::getInstance().getDisplays().getMainDisplay().userArea);

		announcement_->addToDesktop(ComponentPeer::windowIsTemporary);
		announcement_->setVisible(true);
		announcement_->setBounds(bounds);
		announcement_->goToURL(url);
	}
}
