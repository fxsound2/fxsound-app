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
#include "FxTheme.h"
#include <cstring>

namespace
{
    class FxLanguageMenuItem : public PopupMenu::CustomComponent
    {
    public:
        FxLanguageMenuItem(String text, Font font)
            : PopupMenu::CustomComponent(true), text_(std::move(text)), font_(std::move(font))
        {
        }

        void getIdealSize(int& idealWidth, int& idealHeight) override
        {
            idealWidth = FxLanguage::WIDTH;
            idealHeight = FxLanguage::HEIGHT;
        }

        void paint(Graphics& g) override
        {
            auto& lf = getLookAndFeel();
            g.fillAll(lf.findColour(isItemHighlighted() ? PopupMenu::highlightedBackgroundColourId
                                                          : PopupMenu::backgroundColourId));
            g.setColour(lf.findColour(PopupMenu::textColourId));
            g.setFont(font_);
            g.drawText(text_, getLocalBounds().reduced(10, 0), Justification::centredLeft, true);
        }

    private:
        String text_;
        Font font_;
    };

    Typeface::Ptr getTypefaceForLanguage(const String& language_code, FxTheme& theme)
    {
        static StringArray cached_keys;
        static Array<Typeface::Ptr> cached_typefaces;

        auto& language_info = FxLanguage::find(language_code);
        String key = language_info.font_600_file != nullptr ? String(language_info.font_600_file) : String();

        int cached_index = cached_keys.indexOf(key);
        if (cached_index >= 0)
        {
            return cached_typefaces[cached_index];
        }

        Typeface::Ptr typeface = language_info.font_600_file != nullptr
            ? theme.loadTypeface(language_info.font_600_file)
            : nullptr;

        if (typeface == nullptr)
        {
            typeface = Typeface::createSystemTypefaceFor(BinaryData::GilroySemibold_ttf, BinaryData::GilroySemibold_ttfSize);
        }

        cached_keys.add(key);
        cached_typefaces.add(typeface);
        return typeface;
    }

    const std::vector<FxLanguageInfo> kLanguages = {
        { "en", L"English", nullptr, 0, nullptr, nullptr, nullptr },
        { "ar", L"\u0627\u0644\u0639\u0631\u0628\u064a\u0629", BinaryData::FxSound_ar_txt, BinaryData::FxSound_ar_txtSize, "IBMPlexSansArabic-Regular.ttf", "IBMPlexSansArabic-Medium.ttf", "IBMPlexSansArabic-Bold.ttf" },
        { "ba", L"bosanski", BinaryData::FxSound_ba_txt, BinaryData::FxSound_ba_txtSize, nullptr, nullptr, nullptr },
        { "bg", L"\u0431\u044a\u043b\u0433\u0430\u0440\u0441\u043a\u0438", BinaryData::FxSound_bg_txt, BinaryData::FxSound_bg_txtSize, nullptr, nullptr, nullptr },
        { "hr", L"hrvatski", BinaryData::FxSound_hr_txt, BinaryData::FxSound_hr_txtSize, nullptr, nullptr, nullptr },
        { "cs", L"\u010cesky", BinaryData::FxSound_cs_txt, BinaryData::FxSound_cs_txtSize, nullptr, nullptr, nullptr },
        { "de", L"Deutsch", BinaryData::FxSound_de_txt, BinaryData::FxSound_de_txtSize, nullptr, nullptr, nullptr },
        { "es", L"Espa\u00f1ol", BinaryData::FxSound_es_txt, BinaryData::FxSound_es_txtSize, nullptr, nullptr, nullptr },
        { "fi", L"Suomi", BinaryData::FxSound_fi_txt, BinaryData::FxSound_fi_txtSize, nullptr, nullptr, nullptr },
        { "fr", L"fran\u00e7ais", BinaryData::FxSound_fr_txt, BinaryData::FxSound_fr_txtSize, nullptr, nullptr, nullptr },
        { "hu", L"Magyar", BinaryData::fxsound_hu_txt, BinaryData::fxsound_hu_txtSize, nullptr, nullptr, nullptr },
        { "id", L"bahasa Indonesia", BinaryData::FxSound_id_txt, BinaryData::FxSound_id_txtSize, nullptr, nullptr, nullptr },
        { "it", L"Italiano", BinaryData::FxSound_it_txt, BinaryData::FxSound_it_txtSize, nullptr, nullptr, nullptr },
        { "ja", L"\u65e5\u672c\u8a9e", BinaryData::FxSound_ja_txt, BinaryData::FxSound_ja_txtSize, "NotoSansJP-Regular.ttf", "NotoSansJP-Medium.ttf", "NotoSansJP-Bold.ttf" },
        { "ko", L"\ud55c\uad6d\uc5b4", BinaryData::FxSound_ko_txt, BinaryData::FxSound_ko_txtSize, "NotoSansKR-Regular.otf", "NotoSansKR-Medium.otf", "NotoSansKR-Medium.otf" },
        { "nl", L"Nederlands", BinaryData::FxSound_nl_txt, BinaryData::FxSound_nl_txtSize, nullptr, nullptr, nullptr },
        { "no", L"Norsk", BinaryData::FxSound_no_txt, BinaryData::FxSound_no_txtSize, nullptr, nullptr, nullptr },
        { "fa", L"\u0641\u0627\u0631\u0633\u06cc", BinaryData::FxSound_fa_txt, BinaryData::FxSound_fa_txtSize, "IBMPlexSansArabic-Regular.ttf", "IBMPlexSansArabic-Medium.ttf", "IBMPlexSansArabic-Bold.ttf" },
        { "pl", L"Polski", BinaryData::FxSound_pl_txt, BinaryData::FxSound_pl_txtSize, nullptr, nullptr, nullptr },
        { "pt", L"Portugu\u00eas", BinaryData::FxSound_pt_txt, BinaryData::FxSound_pt_txtSize, nullptr, nullptr, nullptr },
        { "pt-br", L"portugu\u00eas brasileiro", BinaryData::FxSound_ptbr_txt, BinaryData::FxSound_ptbr_txtSize, nullptr, nullptr, nullptr },
        { "ro", L"Rom\u00e2n\u0103", BinaryData::FxSound_ro_txt, BinaryData::FxSound_ro_txtSize, nullptr, nullptr, nullptr },
        { "ru", L"\u0440\u0443\u0441\u0441\u043a\u0438\u0439", BinaryData::FxSound_ru_txt, BinaryData::FxSound_ru_txtSize, nullptr, nullptr, nullptr },
        { "sl", L"Sloven\u0161\u010dina", BinaryData::FxSound_sl_txt, BinaryData::FxSound_sl_txtSize, nullptr, nullptr, nullptr },
        { "sv", L"svenska", BinaryData::FxSound_sv_txt, BinaryData::FxSound_sv_txtSize, nullptr, nullptr, nullptr },
        { "th", L"\u0e41\u0e1a\u0e1a\u0e44\u0e17\u0e22", BinaryData::FxSound_th_txt, BinaryData::FxSound_th_txtSize, "NotoSansThai-Regular.ttf", "NotoSansThai-Medium.ttf", "NotoSansThai-Medium.ttf" },
        { "tr", L"T\u00fcrk", BinaryData::FxSound_tr_txt, BinaryData::FxSound_tr_txtSize, nullptr, nullptr, nullptr },
        { "ua", L"\u0443\u043a\u0440\u0430\u0457\u043d\u0441\u044c\u043a\u0430", BinaryData::FxSound_ua_txt, BinaryData::FxSound_ua_txtSize, nullptr, nullptr, nullptr },
        { "vi", L"Ti\u1ebfng Vi\u1ec7t", BinaryData::FxSound_vi_txt, BinaryData::FxSound_vi_txtSize, "MontserratAlternates-Regular.ttf", "MontserratAlternates-Medium.ttf", "MontserratAlternates-Bold.ttf" },
        { "zh-CN", L"\u7b80\u4f53\u4e2d\u6587", BinaryData::FxSound_zhCN_txt, BinaryData::FxSound_zhCN_txtSize, "NotoSansSC-Regular.otf", "NotoSansSC-Medium.otf", "NotoSansSC-Medium.otf" },
        { "zh-TW", L"\u7e41\u9ad4\u4e2d\u6587", BinaryData::FxSound_zhTW_txt, BinaryData::FxSound_zhTW_txtSize, "NotoSansTC-Regular.ttf", "NotoSansTC-Medium.ttf", "NotoSansTC-Medium.ttf" },
    };
}

const FxLanguageInfo& FxLanguage::find(const String& language_code)
{
    const FxLanguageInfo* best = nullptr;
    const FxLanguageInfo* default_entry = nullptr;

    for (auto& entry : kLanguages)
    {
        if (String(entry.code) == "en")
        {
            default_entry = &entry;
        }

        if (language_code.startsWithIgnoreCase(entry.code)
            && (best == nullptr || std::strlen(entry.code) > std::strlen(best->code)))
        {
            best = &entry;
        }
    }

    jassert(default_entry != nullptr);
    return best != nullptr ? *best : *default_entry;
}

FxLanguage::FxLanguage()
{
    language_box_.setJustificationType(Justification::centredLeft);
    language_box_.setMouseCursor(MouseCursor::PointingHandCursor);

    setSize(WIDTH, HEIGHT);
    language_box_.setSize(WIDTH, HEIGHT);

    auto& theme = dynamic_cast<FxTheme&>(LookAndFeel::getDefaultLookAndFeel());
    float item_font_height = theme.getComboBoxFont(language_box_).getHeight();

    int id = 1;
    for (auto& entry : kLanguages)
    {
        language_box_.addItem(String(entry.display_name), id++);
    }

    addAndMakeVisible(&language_box_);

    for (PopupMenu::MenuItemIterator iter(*language_box_.getRootMenu(), true); iter.next();)
    {
        auto& item = iter.getItem();
        int index = item.itemID - 1;
        if (index < 0 || index >= (int) kLanguages.size())
        {
            continue;
        }

        auto& entry = kLanguages[(size_t) index];
        Font font = Font(getTypefaceForLanguage(entry.code, theme)).withHeight(item_font_height);
        item.customComponent = new FxLanguageMenuItem(String(entry.display_name), font);
    }

    language_box_.onChange = [this]() {
        this->onLanguageSelected();
    };
}

void FxLanguage::resized()
{
    language_box_.setBounds(getLocalBounds());
}

void FxLanguage::lookAndFeelChanged()
{
    Component::SafePointer<ComboBox> combo_box(&language_box_);
    MessageManager::callAsync([combo_box]() {
        if (combo_box != nullptr)
        {
            combo_box->resized();
        }
    });
}

void FxLanguage::setSelectedLanguage(const String& language_code)
{
    auto& entry = FxLanguage::find(language_code);
    int index = (int) (&entry - kLanguages.data());
    language_box_.setSelectedId(index + 1, NotificationType::dontSendNotification);
}

void FxLanguage::onLanguageSelected()
{
    int selected_index = language_box_.getSelectedId() - 1;
    if (selected_index < 0 || selected_index >= (int) kLanguages.size())
    {
        return;
    }

    if (onLanguageChanged)
    {
        onLanguageChanged(kLanguages[(size_t) selected_index].code);
    }
}
