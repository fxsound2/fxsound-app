/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "GUI/AnnouncementComponent.h"
#include "GUI/AnimationDemoComponent.h"
#include "GUI/UITheme.h"
#include "GUI/FxMinView.h"
#include "GUI/FxExpandedView.h"
#include "Utils/Settings/Settings.h"
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
constexpr wchar_t CHECK_ANNOUNCEMENT_URL[] = L"https://www.fxsound.com/tmp/check_announcement.json";
constexpr char ATTRIB_URL[] = "url";
constexpr char ATTRIB_WIDTH[] = "width";
constexpr char ATTRIB_HEIGHT[] = "height";

class MainComponent : public Component, public FxView::ResizeViewListener
{
public:
    //==============================================================================
    MainComponent(const String& name);
    ~MainComponent();

    enum class ColorScheme {
        Dark,
        Midnight,
        Grey,
        Light
    };

    //==============================================================================
	void paint(Graphics& g) override;
	void resized() override;

	void mouseDown(const MouseEvent& e) override;
	void mouseDrag(const MouseEvent& e) override;
	void mouseDoubleClick(const MouseEvent& e) override;

	bool hitTest(int x, int y) override;

	void userTriedToCloseWindow() override;

	void resizeView() override;

private:
	void showAnnouncement();
    //==============================================================================
    // Your private member variables go here...
	std::unique_ptr<UITheme> look_and_feel_;
	std::unique_ptr<AnnouncementComponent> announcement_;
	std::unique_ptr<FxMinView> min_view_;
	std::unique_ptr<FxExpandedView> expanded_view_;
	ComponentDragger dragger_;
	ComponentAnimator animator_;

	FxSound::Settings settings_;
	
	bool show_caption_;
	String caption_;
	double font_size_;
	uint32 font_color_;

	ColorScheme colour_scheme_;

	enum FxViewType {MinView, ExpandedView} fx_view_;
	bool resize_view_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
