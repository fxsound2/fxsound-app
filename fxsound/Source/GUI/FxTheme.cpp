/*
FxSound
Copyright (C) 2023  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <JuceHeader.h>
#include "FxTheme.h"

//==============================================================================
FxTheme::FxTheme() : LookAndFeel_V4(getFxColourScheme())
{
	setColour(ComboBox::ColourIds::arrowColourId, Colour(0xe63462).withAlpha(1.0f));
	setColour(ComboBox::ColourIds::backgroundColourId, Colour(0x000000).withAlpha(1.0f));
	setColour(ComboBox::ColourIds::outlineColourId, Colour(0x000000).withAlpha(1.0f));
	setColour(ComboBox::ColourIds::focusedOutlineColourId, Colour(0x000000).withAlpha(1.0f));
	setColour(ComboBox::ColourIds::textColourId, Colour(0xb1b1b1).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::backgroundColourId, Colour(0x000000).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::outlineColourId, Colour(0x000000).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(0x000000).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::textColourId, Colour(0xb1b1b1).withAlpha(1.0f));
	setColour(TextEditor::ColourIds::highlightedTextColourId, Colour(0xffffff).withAlpha(1.0f));
	setColour(TextButton::ColourIds::buttonColourId, Colour(0xd51535).withAlpha(1.0f));
	setColour(TextButton::ColourIds::buttonOnColourId, Colour(0xd51535).withAlpha(1.0f));
	setColour(TextButton::ColourIds::textColourOffId, Colour(0xffffff).withAlpha(1.0f));
	setColour(TextButton::ColourIds::textColourOnId, Colour(0xffffff).withAlpha(1.0f));
	setColour(HyperlinkButton::ColourIds::textColourId, Colour(0xffffff).withAlpha(1.0f));
	setColour(CaretComponent::ColourIds::caretColourId, Colour(0xb1b1b1).withAlpha(1.0f));
	setColour(PopupMenu::ColourIds::backgroundColourId, Colour(0x000000).withAlpha(1.0f));
	setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, Colour(0xe63462).withAlpha(1.0f));
    setColour(Slider::ColourIds::rotarySliderOutlineColourId, Colour(0xe33250).withAlpha(0.2f));
    setColour(Slider::ColourIds::rotarySliderFillColourId, Colour(0xe33250).withAlpha(1.0f));

	drop_down_arrow_ = Drawable::createFromImageData(BinaryData::dropdown_arrow_hover_svg, BinaryData::dropdown_arrow_hover_svgSize);
	slider_thumb_ = Drawable::createFromImageData(BinaryData::Slider_Thumb_svg, BinaryData::Slider_Thumb_svgSize);
    drop_down_arrow_grey_ = Drawable::createFromImageData(BinaryData::dropdown_arrow_bw_svg, BinaryData::dropdown_arrow_bw_svgSize);
    slider_thumb_grey_ = Drawable::createFromImageData(BinaryData::Slider_Thumb_bw_svg, BinaryData::Slider_Thumb_bw_svgSize);
}

LookAndFeel_V4::ColourScheme FxTheme::getFxColourScheme()
{
	return { Colour(0x181818).withAlpha(1.0f), Colour(0x181818).withAlpha(1.0f), Colour(0x383838).withAlpha(1.0f),
			 Colour(0x2B2B2B).withAlpha(1.0f), Colour(0xB1B1B1).withAlpha(1.0f), Colour(0x0).withAlpha(0.2f),
			 0xffffffff, Colour(0x0c0c0c).withAlpha(1.0f), 0xffffffff };
}

Label* FxTheme::createComboBoxTextBox(ComboBox& box)
{
	auto label = LookAndFeel_V4::createComboBoxTextBox(box);
	label->setMouseCursor(MouseCursor::PointingHandCursor);
	return label;
}

Font FxTheme::getComboBoxFont(ComboBox&)
{
	return Font(font_600_).withHeight(17.0f);
}

void FxTheme::positionComboBoxText(ComboBox& box, Label& label)
{
	label.setMinimumHorizontalScale(1.0);
	LookAndFeel_V4::positionComboBoxText(box, label);
	label.setBounds(label.getBounds().withX(20));
}

void FxTheme::drawComboBox(Graphics& g, int width, int height, bool,
							int, int, int, int, ComboBox& box)
{
	auto cornerSize = (float) height / 5;
	Rectangle<int> boxBounds(0, 0, width, height);

	g.setColour(box.findColour(ComboBox::backgroundColourId));
	g.fillRoundedRectangle(boxBounds.toFloat(), cornerSize);

	if (box.hasKeyboardFocus(true))
	{
		g.setColour(Colour(0xf7546f).withAlpha(0.2f));
		g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);
	}
	else
	{
		g.setColour(box.findColour(ComboBox::outlineColourId));
		g.drawRoundedRectangle(boxBounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);
	}
	

	g.setColour(box.findColour(ComboBox::arrowColourId).withAlpha((box.isEnabled() ? 1.0f : 0.2f)));
    if (box.isEnabled())
	    drop_down_arrow_->drawWithin(g, Rectangle<float>(width - 32, 0, 12, height), { RectanglePlacement::centred }, 1.0f);
    else
        drop_down_arrow_grey_->drawWithin(g, Rectangle<float>(width - 32, 0, 12, height), { RectanglePlacement::centred }, 1.0f);
}

void FxTheme::drawLinearSlider(Graphics& g, int x, int y, int width, int height,
								float sliderPos, float minSliderPos, float maxSliderPos,
								const Slider::SliderStyle style, Slider& slider)
{
	if (style == Slider::LinearVertical)
	{
		float dash_lengths[] = { 5, 2 };

		auto radius = getSliderThumbRadius(slider);

        Colour colour1;
        colour1 = Colour(0xe33250).withAlpha(0.4f);
        Colour colour2;
        colour2 = Colour(0xf3f3f3).withAlpha(0.4f);

        if (!slider.isEnabled())
        {
            colour1 = colour1.withSaturation(0.0);
            colour2 = colour2.withSaturation(0.0);
        }

		g.setGradientFill(ColourGradient(colour1, { 0, 0 }, colour2, { 0, (float)height }, false));
		g.drawDashedLine(Line<float>(x+width/2, y, x+width/2, y+height), dash_lengths, _countof(dash_lengths), 1.0f);

        if (slider.isEnabled())
		    slider_thumb_->drawWithin(g, Rectangle<float>(x+width/2-radius, sliderPos-radius, radius*2, radius*2), { RectanglePlacement::centred }, 1.0f);
        else
            slider_thumb_grey_->drawWithin(g, Rectangle<float>(x + width / 2 - radius, sliderPos - radius, radius * 2, radius * 2), { RectanglePlacement::centred }, 1.0f);

        if (slider.getThumbBeingDragged() >= 0 || slider.hasKeyboardFocus(true))
        {
            Colour colour = Colour(0xf7546f).withAlpha(0.1f);
            g.setFillType(colour);
            g.fillRoundedRectangle(juce::Rectangle<float>(x + (width - SLIDER_THUMB_RADIUS*4) / 2, y, SLIDER_THUMB_RADIUS * 4, height).expanded(0, SLIDER_THUMB_RADIUS), 20);
        }
	}
	else if (style == Slider::LinearHorizontal)
	{
		auto radius = getSliderThumbRadius(slider);

        Colour colour = Colour(0xe33250).withAlpha(0.2f);
        if (!slider.isEnabled())
        {
            colour = colour.withSaturation(0.0);
        }

		g.setFillType(colour);
		g.fillRoundedRectangle(x, y+(height-3)/2, width, 3, 5.6f);

        colour = Colour(0xe33250).withAlpha(1.0f);
        if (!slider.isEnabled())
        {
            colour = colour.withSaturation(0.0);
        }

		g.setFillType(colour);
		g.fillRoundedRectangle(x, y+(height-3)/2, sliderPos, 3, 5.6f);

        if (slider.isEnabled())
		    slider_thumb_->drawWithin(g, Rectangle<float>(sliderPos-radius, y+height/2-radius, radius*2, radius*2), { RectanglePlacement::centred }, 1.0f);
        else
            slider_thumb_grey_->drawWithin(g, Rectangle<float>(sliderPos - radius, y + height / 2 - radius, radius * 2, radius * 2), { RectanglePlacement::centred }, 1.0f);
	
		if (slider.hasKeyboardFocus(true))
		{
			Colour colour = Colour(0xf7546f).withAlpha(0.1f);
			g.setFillType(colour);
			g.fillRoundedRectangle(juce::Rectangle<float>(x, y, width, height).expanded(SLIDER_THUMB_RADIUS/2, SLIDER_THUMB_RADIUS/2), height+SLIDER_THUMB_RADIUS);
		}
	}
	else
	{
		LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
	}
}

void FxTheme::drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
                                const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider)
{
    auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(Slider::rotarySliderFillColourId);

    if (!slider.isEnabled())
    {
        outline = outline.withSaturation(0.0);
        fill = fill.withSaturation(0.0);
    }

    auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(2);

    auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 5.0f;
    auto arcRadius = radius - lineW * 0.5f;

    Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
        bounds.getCentreY(),
        arcRadius,
        arcRadius,
        0.0f,
        rotaryStartAngle,
        rotaryEndAngle,
        true);

    g.setColour(outline);
    g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    Path valueArc;
    valueArc.addCentredArc(bounds.getCentreX(),
        bounds.getCentreY(),
        arcRadius,
        arcRadius,
        0.0f,
        rotaryStartAngle,
        toAngle,
        true);

    g.setColour(fill);
    g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
	if (slider.hasKeyboardFocus(true))
	{
		DropShadow shadow;
		shadow.colour = Colour(0xf7546f).withAlpha(0.1f);
		shadow.drawForPath(g, backgroundArc);
	}

    auto thumbRadius = getSliderThumbRadius(slider);
    Point<float> thumbPoint(bounds.getCentreX() + arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
                            bounds.getCentreY() + arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));
    auto thumbX = thumbPoint.getX() - thumbRadius;
    auto thumbY = thumbPoint.getY() - thumbRadius;

    if (slider.isEnabled())
        slider_thumb_->drawWithin(g, Rectangle<float>(thumbX, thumbY, thumbRadius*2, thumbRadius*2), { RectanglePlacement::centred }, 1.0f);
    else
        slider_thumb_grey_->drawWithin(g, Rectangle<float>(thumbX, thumbY, thumbRadius * 2, thumbRadius * 2), { RectanglePlacement::centred }, 1.0f);
}

int FxTheme::getSliderThumbRadius(Slider& slider)
{
    if (slider.getSliderStyle() == Slider::Rotary)
    {
        return ROTARY_SLIDER_THUMB_RADIUS;
    }
    else
    {
        return SLIDER_THUMB_RADIUS;
    }
}

Slider::SliderLayout FxTheme::getSliderLayout(Slider& slider)
{
	auto layout = LookAndFeel_V4::getSliderLayout(slider);
	auto style = slider.getSliderStyle();

	if (style == Slider::LinearVertical)
	{
		auto y = layout.sliderBounds.getY() + (SLIDER_THUMB_RADIUS*2);
		auto height = layout.sliderBounds.getHeight() - (SLIDER_THUMB_RADIUS*2);
		layout.sliderBounds.setY(y);
		layout.sliderBounds.setHeight(height);
	}
	else if (style == Slider::LinearHorizontal)
	{
		auto width = layout.sliderBounds.getWidth() - (SLIDER_THUMB_RADIUS*3);
		layout.sliderBounds.setWidth(width);
	}

	return layout;
}

void FxTheme::drawPopupMenuItem(Graphics& g, const juce::Rectangle<int>& area, bool is_separator, bool is_active,
								bool is_highlighted, bool is_ticked, bool has_submenu, const String& text,
								const String& shortcut_key_text, const Drawable* icon, const Colour* text_colour)
{
	LookAndFeel_V4::drawPopupMenuItem(g, area, is_separator, is_active, is_highlighted|is_ticked, false,
										has_submenu, text, shortcut_key_text, icon, text_colour);
	
	if (is_ticked)
	{
		g.setColour(findColour(PopupMenu::textColourId).withAlpha(1.0f));
		g.drawRect(area.toFloat());
	}
}

Font FxTheme::getPopupMenuFont()
{
	return Font(font_600_).withHeight(17.0f);
}

void FxTheme::preparePopupMenuWindow(Component& new_window)
{
	new_window.setMouseCursor(MouseCursor::PointingHandCursor);
	auto children = new_window.getChildren();
	for (auto child : children)
	{
		child->setMouseCursor(MouseCursor::PointingHandCursor);
	}
}

void FxTheme::loadFont(String language)
{
    if (language.startsWithIgnoreCase("en"))
    {
        font_400_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyRegular_ttf, BinaryData::GilroyRegular_ttfSize);
        font_600_ = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
        font_700_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyBold_ttf, BinaryData::GilroyBold_ttfSize);
    }
    else if (language.startsWithIgnoreCase("ko"))
    {
        font_400_ = loadTypeface("NotoSansKR-Regular.otf");
        font_600_ = loadTypeface("NotoSansKR-Medium.otf");
        font_700_ = loadTypeface("NotoSansKR-Medium.otf");
    }
    else if (language.startsWithIgnoreCase("zh"))
    {
        font_400_ = loadTypeface("NotoSansSC-Regular.otf");
        font_600_ = loadTypeface("NotoSansSC-Medium.otf");
        font_700_ = loadTypeface("NotoSansSC-Medium.otf");
    }
	else if (language.startsWithIgnoreCase("th"))
	{
		font_400_ = loadTypeface("NotoSansThai-Regular.ttf");
		font_600_ = loadTypeface("NotoSansThai-Medium.ttf");
		font_700_ = loadTypeface("NotoSansThai-Medium.ttf");
	}
    else if (language.startsWithIgnoreCase("vi"))
    {
        font_400_ = loadTypeface("MontserratAlternates-Regular.ttf");
        font_600_ = loadTypeface("MontserratAlternates-Medium.ttf");
        font_700_ = loadTypeface("MontserratAlternates-Bold.ttf");
    }
	else if (language.startsWithIgnoreCase("ja"))
	{
		font_400_ = loadTypeface("NotoSansJP-Regular.ttf");
		font_600_ = loadTypeface("NotoSansJP-Medium.ttf");
		font_700_ = loadTypeface("NotoSansJP-Bold.ttf");
	}
	else if (language.startsWithIgnoreCase("ar"))
	{
		font_400_ = loadTypeface("IBMPlexSansArabic-Regular.ttf");
		font_600_ = loadTypeface("IBMPlexSansArabic-Medium.ttf");
		font_700_ = loadTypeface("IBMPlexSansArabic-Bold.ttf");
	}
    else
    {
        font_400_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyRegular_ttf, BinaryData::GilroyRegular_ttfSize);
        font_600_ = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
        font_700_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyBold_ttf, BinaryData::GilroyBold_ttfSize);
    }
	
    if (font_400_ == nullptr)
    {
        font_400_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyRegular_ttf, BinaryData::GilroyRegular_ttfSize);
    }
    if (font_600_ == nullptr)
    {
        font_600_ = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
    }
    if (font_700_ == nullptr)
    {
        font_700_ = Typeface::createSystemTypefaceFor(BinaryData::GilroyBold_ttf, BinaryData::GilroyBold_ttfSize);
    }

    setDefaultSansSerifTypeface(font_600_);
}

Font FxTheme::getTextButtonFont(TextButton&, int button_height)
{
	return Font(font_600_.get()).withHeight(jmin(17.0f, (float)button_height));
}

Font FxTheme::getNormalFont()
{
	return Font(font_600_).withHeight(17.0f);
}

Font FxTheme::getSmallFont()
{
	return Font(font_400_).withHeight(14.0f);
}

Font FxTheme::getTitleFont()
{
	return Font(font_700_).withHeight(17.0f);
}

Typeface::Ptr FxTheme::getDefaultTypeface()
{
    return font_400_;
}

Rectangle<int> FxTheme::getTooltipBounds(const String& tipText, Point<int> screenPos, Rectangle<int> parentArea)
{
    const TextLayout tl(layoutTooltipText(tipText, Colours::black));

    auto w = (int)(tl.getWidth() + 20.0f);
    auto h = (int)(tl.getHeight() + 12.0f);

    return Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 18) : screenPos.x + 36,
        screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 12) : screenPos.y + 12,
        w, h)
        .constrainedWithin(parentArea);
}

void FxTheme::drawTooltip(Graphics& g, const String& text, int width, int height)
{
    Rectangle<int> bounds(width, height);
    auto cornerSize = 5.0f;

    g.setColour(findColour(TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle(bounds.toFloat(), cornerSize);

    g.setColour(findColour(TooltipWindow::outlineColourId));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f, 0.5f), cornerSize, 1.0f);

    layoutTooltipText(text, findColour(TooltipWindow::textColourId))
        .draw(g, bounds.toFloat().reduced(10, 0));
}

void FxTheme::drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g,
										int w, int h, int titleSpaceX, int titleSpaceW,
										const Image* icon, bool drawTitleTextOnLeft)
{
	if (w * h == 0)
		return;

	auto isActive = window.isActiveWindow();

	g.setColour(getCurrentColourScheme().getUIColour(ColourScheme::widgetBackground));
	g.fillAll();

	Font font(12.0f, Font::plain);
	g.setFont(font);

	auto textW = font.getStringWidth(window.getName());
	auto iconW = 0;
	auto iconH = 0;

	if (icon != nullptr)
	{
		iconH = static_cast<int> (font.getHeight());
		iconW = icon->getWidth() * iconH / icon->getHeight() + 4;
	}

	textW = jmin(titleSpaceW, textW + iconW);
	auto textX = drawTitleTextOnLeft ? titleSpaceX
		: jmax(titleSpaceX, (w - textW) / 2);

	if (textX + textW > titleSpaceX + titleSpaceW)
		textX = titleSpaceX + titleSpaceW - textW;

	if (icon != nullptr)
	{
		g.setOpacity(isActive ? 1.0f : 0.6f);
		g.drawImageWithin(*icon, textX, (h - iconH) / 2, iconW, iconH,
			RectanglePlacement::centred, false);
		textX += iconW;
		textW -= iconW;
	}

	if (window.isColourSpecified(DocumentWindow::textColourId) || isColourSpecified(DocumentWindow::textColourId))
		g.setColour(window.findColour(DocumentWindow::textColourId));
	else
		g.setColour(getCurrentColourScheme().getUIColour(ColourScheme::defaultText));

	g.drawText(window.getName(), textX, 0, textW, h, Justification::centredLeft, true);
}

class FxTheme_DocumentWindowButton : public Button
{
public:
	FxTheme_DocumentWindowButton(const String& name, Colour c, const Path& normal, const Path& toggled)
		: Button(name), colour(c), normalShape(normal), toggledShape(toggled)
	{
	}

	void paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
	{
		auto background = Colours::grey;

		if (auto* rw = findParentComponentOfClass<ResizableWindow>())
			if (auto lf = dynamic_cast<LookAndFeel_V4*> (&rw->getLookAndFeel()))
				background = lf->getCurrentColourScheme().getUIColour(LookAndFeel_V4::ColourScheme::widgetBackground);

		g.fillAll(background);

		g.setColour((!isEnabled() || shouldDrawButtonAsDown) ? colour.withAlpha(0.6f)
			: colour);

		if (shouldDrawButtonAsHighlighted)
		{
			g.fillAll();
			g.setColour(background);
		}

		auto& p = getToggleState() ? toggledShape : normalShape;

		auto reducedRect = Justification(Justification::centred)
			.appliedToRectangle(Rectangle<int>(12, 12), getLocalBounds())
			.toFloat();

		g.fillPath(p, p.getTransformToScaleToFit(reducedRect, true));
	}

private:
	Colour colour;
	Path normalShape, toggledShape;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FxTheme_DocumentWindowButton)
};

Button* FxTheme::createDocumentWindowButton(int buttonType)
{
	Path shape;
	auto crossThickness = 0.15f;

	if (buttonType == DocumentWindow::closeButton)
	{
		shape.addLineSegment({ 0.0f, 0.0f, 1.0f, 1.0f }, crossThickness);
		shape.addLineSegment({ 1.0f, 0.0f, 0.0f, 1.0f }, crossThickness);

		return new FxTheme_DocumentWindowButton("close", Colour(0xffffffff), shape, shape);
	}

	if (buttonType == DocumentWindow::minimiseButton)
	{
		shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);

		return new FxTheme_DocumentWindowButton("minimise", Colour(0xffffffff), shape, shape);
	}

	if (buttonType == DocumentWindow::maximiseButton)
	{
		shape.addLineSegment({ 0.5f, 0.0f, 0.5f, 1.0f }, crossThickness);
		shape.addLineSegment({ 0.0f, 0.5f, 1.0f, 0.5f }, crossThickness);

		Path fullscreenShape;
		fullscreenShape.startNewSubPath(45.0f, 100.0f);
		fullscreenShape.lineTo(0.0f, 100.0f);
		fullscreenShape.lineTo(0.0f, 0.0f);
		fullscreenShape.lineTo(100.0f, 0.0f);
		fullscreenShape.lineTo(100.0f, 45.0f);
		fullscreenShape.addRectangle(45.0f, 45.0f, 100.0f, 100.0f);
		PathStrokeType(30.0f).createStrokedPath(fullscreenShape, fullscreenShape);

		return new FxTheme_DocumentWindowButton("maximise", Colour(0xffffffff), shape, fullscreenShape);
	}

	jassertfalse;
	return nullptr;
}

TextLayout FxTheme::layoutTooltipText(const String& text, Colour colour) noexcept
{
    const float tooltipFontSize = 14.0f;
    const int maxToolTipWidth = 400;

    AttributedString s;
    s.setWordWrap(AttributedString::WordWrap::byWord);
    s.setJustification(Justification::centredLeft);
    s.append(text, getNormalFont().withHeight(tooltipFontSize), colour);

    TextLayout tl;
    tl.createLayout(s, (float)maxToolTipWidth);
    return tl;
}

Typeface::Ptr FxTheme::loadTypeface(String fileName)
{
    MemoryBlock fontBuffer;
    String filePath = File::addTrailingSeparator(File::getCurrentWorkingDirectory().getFullPathName());
    File fontFile = File(filePath+fileName);
    if (fontFile.exists())
    {
        if (fontFile.loadFileAsData(fontBuffer))
        {
            return Typeface::createSystemTypefaceFor(fontBuffer.getData(), fontBuffer.getSize());
        }
    }
        
    return nullptr;
}