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

#ifndef FXTHEME_H
#define FXTHEME_H

#include <JuceHeader.h>

//==============================================================================
/*
*/

enum FxThemeMode : int {Dark=0, Light, NumModes};
enum FxColor : int { WindowBackground, WidgetBackground, MenuBackground, Outline, DefaultText, DefaultFill, HighlightedText, HighlightedFill, MenuText,
					 ComboBoxBackground, TextButtonBackground, ImageButton, HintText, ValidTextBorder, InvalidTextBorder, ControlBackground, SliderTrack, SliderHighlight,
					 GraphHigh, GraphLow, EqStart, EqEnd, VerticalSliderLow, MenuHighlightBackground, PanelBackground, NumColors};
enum FxImage : int { DefaultLogo, HighlightedLogo, IconLogo,
					 PowerOnButton, PowerOffButton, DonateButton, DonateButtonHover, MenuButton, MenuButtonHover,
					 MinimizeButton, MinimizeButtonHover, MaximizeButton, MaximizeButtonHover, MinimizeWindowButton, MinimizeWindowButtonHover,
				     ArrowNext, ArrowNextBW, ArrowPrev, ArrowPrevBW, DropDownArrow, DropDownArrowHover,
	                 SliderThumb, SliderThumbBW, NumImages };

class FxTheme : public LookAndFeel_V4
{
public:
    static constexpr int WINDOW_CORNER_RADIUS = 21;
	static constexpr int TITLE_BAR_HEIGHT = 57;
	static constexpr int SLIDER_THUMB_RADIUS = 8;
    static constexpr int ROTARY_SLIDER_THUMB_RADIUS = 5;

	FxTheme();
	~FxTheme() = default;

	LookAndFeel_V4::ColourScheme getFxColourScheme();

	Label* createComboBoxTextBox(ComboBox& box) override;
	Font getComboBoxFont(ComboBox& box) override;
	void positionComboBoxText(ComboBox& box, Label& label) override;
	void drawComboBox(Graphics& g, int width, int height, bool,
						int, int, int, int, ComboBox& box) override;

	void drawLinearSlider(Graphics& g, int x, int y, int width, int height,
							float sliderPos, float minSliderPos, float maxSliderPos,
							const Slider::SliderStyle style, Slider& slider) override;
    void drawRotarySlider(Graphics&, int x, int y, int width, int height,
                            float sliderPosProportional, float rotaryStartAngle,
                            float rotaryEndAngle, Slider&) override;
	int getSliderThumbRadius(Slider& slider) override;
	Slider::SliderLayout getSliderLayout(Slider& slider);

	void drawPopupMenuItem(Graphics& g, const juce::Rectangle<int>& area, bool is_separator, bool is_active,
						   bool is_highlighted, bool is_ticked, bool has_submenu, const String& text,
						   const String& shortcut_key_text, const Drawable* icon, const Colour* text_colour) override;
	Font getPopupMenuFont() override;
	void preparePopupMenuWindow(Component& new_window) override;

	Font getTextButtonFont(TextButton&, int button_height) override;

    juce::Rectangle<int> getTooltipBounds(const String& tipText, Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawTooltip(Graphics&, const String& text, int width, int height) override;

	void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW,
									   const Image* icon, bool drawTitleTextOnLeft) override;
	Button* createDocumentWindowButton(int buttonType) override;

    void loadFont(String language);
	Font getNormalFont();
	Font getSmallFont();
	Font getTitleFont();
    Typeface::Ptr getDefaultTypeface();

	static FxThemeMode getThemeMode();
	static void setThemeMode(FxThemeMode theme_mode);
	static uint32 getColor(FxColor color);
	static const char* getImage(FxImage image);
	static const int getImageSize(FxImage image);

private:
    TextLayout layoutTooltipText(const String& text, Colour colour) noexcept;
    Typeface::Ptr loadTypeface(String fileName);

	std::unique_ptr<Drawable> drop_down_arrow_;
	std::unique_ptr<Drawable> slider_thumb_;
    std::unique_ptr<Drawable> drop_down_arrow_grey_;
    std::unique_ptr<Drawable> slider_thumb_grey_;

	Typeface::Ptr font_400_;
	Typeface::Ptr font_600_;
	Typeface::Ptr font_700_;

	static const uint32 theme_colors_[FxThemeMode::NumModes][FxColor::NumColors];
	static const char* theme_images_[FxThemeMode::NumModes][FxImage::NumImages];
	static const int theme_image_sizes_[FxThemeMode::NumModes][FxImage::NumImages];
	static FxThemeMode theme_mode_;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxTheme)
};

#define FXCOLOR(color) (FxTheme::getColor(FxColor::color))
#define FXIMAGE(image) (FxTheme::getImage(FxImage::image))
#define FXIMAGESIZE(image) (FxTheme::getImageSize(FxImage::image))

#endif
