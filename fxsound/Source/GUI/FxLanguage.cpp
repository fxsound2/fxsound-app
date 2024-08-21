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

#include "FxLanguage.h"
#include "FxController.h"
#include "FxTheme.h"

FxLanguage::FxLanguage() : next_button_("next", DrawableButton::ButtonStyle::ImageFitted), prev_button_("prev", DrawableButton::ButtonStyle::ImageFitted)
{
    languages_ = { "en", "ar", "ba", "hr", "de", "es", "fr", "hu", "id", "it", "ja", "ko", "nl", "pl", "pt", "pt-br", "ro", "ru", "sv", "th", "tr", "vi", "zh"};

    language_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
    language_.setJustificationType(Justification::centred);

    auto next_normal = Drawable::createFromImageData(BinaryData::arrow_next_svg, BinaryData::arrow_next_svgSize);
    auto next_disabled = Drawable::createFromImageData(BinaryData::arrow_next_bw_svg, BinaryData::arrow_next_bw_svgSize);
    next_button_.setImages(next_normal.get(), nullptr, next_disabled.get());
    next_button_.setMouseCursor(MouseCursor::PointingHandCursor);

    auto prev_normal = Drawable::createFromImageData(BinaryData::arrow_prev_svg, BinaryData::arrow_prev_svgSize);
    auto prev_disabled = Drawable::createFromImageData(BinaryData::arrow_prev_bw_svg, BinaryData::arrow_prev_bw_svgSize);
    prev_button_.setImages(prev_normal.get(), nullptr, prev_disabled.get());
    prev_button_.setMouseCursor(MouseCursor::PointingHandCursor);
    
    setSize(WIDTH, HEIGHT);

    prev_button_.setBounds(10, (HEIGHT - BUTTON_HEIGHT) / 2, BUTTON_WIDTH, BUTTON_HEIGHT);
    next_button_.setBounds(WIDTH - BUTTON_WIDTH - 10, (HEIGHT - BUTTON_HEIGHT) / 2, BUTTON_WIDTH, BUTTON_HEIGHT);
    language_.setBounds(prev_button_.getRight(), (HEIGHT - LABEL_HEIGHT)/2, next_button_.getX() - prev_button_.getRight(), LABEL_HEIGHT);

    addAndMakeVisible(&prev_button_);
    addAndMakeVisible(&language_);
    addAndMakeVisible(&next_button_);

    prev_button_.onClick = [this]() {
        this->onPrevLanguage();
    };

    next_button_.onClick = [this]() {
        this->onNextLanguage();
    };

    String language_code = FxController::getInstance().getLanguage();
    language_.setText(FxController::getInstance().getLanguageName(language_code), NotificationType::dontSendNotification);

    language_index_ = -1;
    auto i = 0;
    for (auto lng : languages_)
    {        
        if (language_code.startsWith(lng))
        {
            language_index_ = i;
        }

        i++;
    }
}

void FxLanguage::paint(Graphics& g)
{
    g.setFillType(FillType(Colour(0x0f0f0f).withAlpha(1.0f)));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
}

void FxLanguage::onNextLanguage()
{
    if (++language_index_ >= languages_.size())
    {
        language_index_ = 0;
    }

    String language_code = languages_[language_index_];
    FxController::getInstance().setLanguage(language_code);

    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
    language_.setFont(theme.getNormalFont());

    language_.setText(FxController::getInstance().getLanguageName(language_code), NotificationType::dontSendNotification);
    
}

void FxLanguage::onPrevLanguage()
{    
    if (--language_index_ < 0)
    {
        language_index_ = languages_.size() - 1;
    }

    String language_code = languages_[language_index_];
    FxController::getInstance().setLanguage(language_code);

    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
    language_.setFont(theme.getNormalFont());

    language_.setText(FxController::getInstance().getLanguageName(language_code), NotificationType::dontSendNotification);
}
