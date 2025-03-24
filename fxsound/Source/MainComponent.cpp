/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent(const String& name) : Component(name)
{
	colour_scheme_ = ColorScheme::Dark;
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
	// unique_ptr will automatically handle cleanup of look_and_feel_
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
	switch (colour_scheme_) {
		case ColorScheme::Dark:
			colour_scheme_ = ColorScheme::Midnight;
			look_and_feel_->setColourScheme(LookAndFeel_V4::getMidnightColourScheme());
			break;
		case ColorScheme::Midnight:
			colour_scheme_ = ColorScheme::Grey;
			look_and_feel_->setColourScheme(LookAndFeel_V4::getGreyColourScheme());
			break;
		case ColorScheme::Grey:
			colour_scheme_ = ColorScheme::Light;
			look_and_feel_->setColourScheme(LookAndFeel_V4::getLightColourScheme());
			break;
		case ColorScheme::Light:
			colour_scheme_ = ColorScheme::Dark;
			look_and_feel_->setColourScheme(LookAndFeel_V4::getDarkColourScheme());
			break;
	}
	repaint();
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
	const int SCREEN_MARGIN = 20;  // Configurable margin from screen edges

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

	// Ensure window stays within screen bounds
	if (x + width + SCREEN_MARGIN > desktop_area.getWidth())
	{
		x = desktop_area.getWidth() - (width + SCREEN_MARGIN);
	}
	if (y + height + SCREEN_MARGIN > desktop_area.getHeight())
	{
		y = desktop_area.getHeight() - (height + SCREEN_MARGIN);
	}

	Rectangle<int> final_bounds(x, y, width, height);
	animator_.animateComponent(this, final_bounds, 1.0, 2000, false, 0, 0);
	resize_view_ = true;
}

void MainComponent::showAnnouncement()
{
	try {
		WebInputStream stream(URL(CHECK_ANNOUNCEMENT_URL), false);
		if (!stream.isExhausted()) {
			var attribs = JSON::parse(stream);
			if (attribs.isObject()) {
				String url = attribs[ATTRIB_URL];
				int width = attribs[ATTRIB_WIDTH];
				int height = attribs[ATTRIB_HEIGHT];

				if (url.isNotEmpty() && width > 0 && height > 0) {
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
		}
	} catch (const std::exception& e) {
		// Log error but don't show to user as this is not critical
		Logger::writeToLog("Failed to show announcement: " + String(e.what()));
	}
}
