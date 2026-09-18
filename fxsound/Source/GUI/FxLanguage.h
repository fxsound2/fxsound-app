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

#pragma once

#include <JuceHeader.h>
#include <vector>

//==============================================================================
/*
    A single supported language: its code, display name, translation data
    (nullptr for English, which uses the built-in strings), and any fonts
    it needs in place of the default Gilroy fonts.
*/
struct FxLanguageInfo
{
    const char* code;
    const wchar_t* display_name;
    const char* translation_data;
    int translation_data_size;

    // Font files (relative to the working directory) this language needs
    // instead of the built-in Gilroy fonts, or nullptr to use the default.
    const char* font_400_file;
    const char* font_600_file;
    const char* font_700_file;
};

//==============================================================================
/*
*/
class FxLanguage : public Component
{
public:
    static constexpr int WIDTH = 180;
    static constexpr int HEIGHT = 30;

    FxLanguage();
    ~FxLanguage() = default;

    // Single source of truth for the set of languages FxSound supports.
    // Anything that needs to know "what languages exist" or "how do I
    // load/name language X" should go through these rather than keeping
    // its own copy of the list.
    static const std::vector<FxLanguageInfo>& getAll();

    // Finds the entry whose code is the longest prefix match of language_code
    // (e.g. "pt-br" resolves to the "pt-br" entry rather than "pt"), or
    // nullptr if no entry matches.
    static const FxLanguageInfo* find(const String& language_code);

private:
    static constexpr int BUTTON_WIDTH = 14;
    static constexpr int BUTTON_HEIGHT = 22;
    static constexpr int LABEL_HEIGHT = 22;

    void paint(Graphics& g) override;

    void onNextLanguage();
    void onPrevLanguage();
    
    Label language_;
    DrawableButton next_button_;
    DrawableButton prev_button_;

    StringArray languages_;
    int language_index_;
};