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

#include "FxLanguage.h"
#include "FxController.h"
#include "FxTheme.h"
#include <cstring>

namespace
{
    const std::vector<FxLanguageInfo> kLanguages = {
        { "en", L"English", nullptr, 0 },
        { "ar", L"\u0627\u0644\u0639\u0631\u0628\u064a\u0629", BinaryData::FxSound_ar_txt, BinaryData::FxSound_ar_txtSize },
        { "ba", L"bosanski", BinaryData::FxSound_ba_txt, BinaryData::FxSound_ba_txtSize },
        { "bg", L"\u0431\u044a\u043b\u0433\u0430\u0440\u0441\u043a\u0438", BinaryData::FxSound_bg_txt, BinaryData::FxSound_bg_txtSize },
        { "hr", L"hrvatski", BinaryData::FxSound_hr_txt, BinaryData::FxSound_hr_txtSize },
        { "cs", L"\u010cesky", BinaryData::FxSound_cs_txt, BinaryData::FxSound_cs_txtSize },
        { "de", L"Deutsch", BinaryData::FxSound_de_txt, BinaryData::FxSound_de_txtSize },
        { "es", L"Espa\u00f1ol", BinaryData::FxSound_es_txt, BinaryData::FxSound_es_txtSize },
        { "fi", L"Suomi", BinaryData::FxSound_fi_txt, BinaryData::FxSound_fi_txtSize },
        { "fr", L"fran\u00e7ais", BinaryData::FxSound_fr_txt, BinaryData::FxSound_fr_txtSize },
        { "hu", L"Magyar", BinaryData::fxsound_hu_txt, BinaryData::fxsound_hu_txtSize },
        { "id", L"bahasa Indonesia", BinaryData::FxSound_id_txt, BinaryData::FxSound_id_txtSize },
        { "it", L"Italiano", BinaryData::FxSound_it_txt, BinaryData::FxSound_it_txtSize },
        { "ja", L"\u65e5\u672c\u8a9e", BinaryData::FxSound_ja_txt, BinaryData::FxSound_ja_txtSize },
        { "ko", L"\ud55c\uad6d\uc5b4", BinaryData::FxSound_ko_txt, BinaryData::FxSound_ko_txtSize },
        { "nl", L"Nederlands", BinaryData::FxSound_nl_txt, BinaryData::FxSound_nl_txtSize },
        { "no", L"Norsk", BinaryData::FxSound_no_txt, BinaryData::FxSound_no_txtSize },
        { "fa", L"\u0641\u0627\u0631\u0633\u06cc", BinaryData::FxSound_fa_txt, BinaryData::FxSound_fa_txtSize },
        { "pl", L"Polski", BinaryData::FxSound_pl_txt, BinaryData::FxSound_pl_txtSize },
        { "pt", L"Portugu\u00eas", BinaryData::FxSound_pt_txt, BinaryData::FxSound_pt_txtSize },
        { "pt-br", L"portugu\u00eas brasileiro", BinaryData::FxSound_ptbr_txt, BinaryData::FxSound_ptbr_txtSize },
        { "ro", L"Rom\u00e2n\u0103", BinaryData::FxSound_ro_txt, BinaryData::FxSound_ro_txtSize },
        { "ru", L"\u0440\u0443\u0441\u0441\u043a\u0438\u0439", BinaryData::FxSound_ru_txt, BinaryData::FxSound_ru_txtSize },
        { "sl", L"Sloven\u0161\u010dina", BinaryData::FxSound_sl_txt, BinaryData::FxSound_sl_txtSize },
        { "sv", L"svenska", BinaryData::FxSound_sv_txt, BinaryData::FxSound_sv_txtSize },
        { "th", L"\u0e41\u0e1a\u0e1a\u0e44\u0e17\u0e22", BinaryData::FxSound_th_txt, BinaryData::FxSound_th_txtSize },
        { "tr", L"T\u00fcrk", BinaryData::FxSound_tr_txt, BinaryData::FxSound_tr_txtSize },
        { "ua", L"\u0443\u043a\u0440\u0430\u0457\u043d\u0441\u044c\u043a\u0430", BinaryData::FxSound_ua_txt, BinaryData::FxSound_ua_txtSize },
        { "vi", L"Ti\u1ebfng Vi\u1ec7t", BinaryData::FxSound_vi_txt, BinaryData::FxSound_vi_txtSize },
        { "zh-CN", L"\u7b80\u4f53\u4e2d\u6587", BinaryData::FxSound_zhCN_txt, BinaryData::FxSound_zhCN_txtSize },
        { "zh-TW", L"\u7e41\u9ad4\u4e2d\u6587", BinaryData::FxSound_zhTW_txt, BinaryData::FxSound_zhTW_txtSize },
    };
}

const std::vector<FxLanguageInfo>& FxLanguage::getAll()
{
    return kLanguages;
}

const FxLanguageInfo* FxLanguage::find(const String& language_code)
{
    const FxLanguageInfo* best = nullptr;

    for (auto& entry : kLanguages)
    {
        if (language_code.startsWithIgnoreCase(entry.code)
            && (best == nullptr || std::strlen(entry.code) > std::strlen(best->code)))
        {
            best = &entry;
        }
    }

    return best;
}

FxLanguage::FxLanguage() : next_button_("next", DrawableButton::ButtonStyle::ImageFitted), prev_button_("prev", DrawableButton::ButtonStyle::ImageFitted)
{
    for (auto& entry : kLanguages)
    {
        languages_.add(entry.code);
    }

    language_.setColour(Label::ColourIds::textColourId, getLookAndFeel().findColour(TextButton::textColourOnId));
    language_.setJustificationType(Justification::centred);

    auto next_normal = Drawable::createFromImageData(FXIMAGE(ArrowNext), FXIMAGESIZE(ArrowNext));
    auto next_disabled = Drawable::createFromImageData(FXIMAGE(ArrowNextBW), FXIMAGESIZE(ArrowNextBW));
    next_button_.setImages(next_normal.get(), nullptr, next_disabled.get());
    next_button_.setMouseCursor(MouseCursor::PointingHandCursor);

    auto prev_normal = Drawable::createFromImageData(FXIMAGE(ArrowPrev), FXIMAGESIZE(ArrowPrev));
    auto prev_disabled = Drawable::createFromImageData(FXIMAGE(ArrowPrevBW), FXIMAGESIZE(ArrowPrevBW));
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
    auto* matched = FxLanguage::find(language_code);
    if (matched != nullptr)
    {
        language_index_ = languages_.indexOf(matched->code);
    }
}

void FxLanguage::paint(Graphics& g)
{
    g.setFillType(FillType(Colour(FXCOLOR(ControlBackground)).withAlpha(1.0f)));
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
