#include "../engine/audio-devices.h"
#include <cassert>
#include <iostream>
int main() {
    using fxsound::jsonString;
    assert(jsonString("") == "\"\"");
    assert(jsonString("plain UID") == "\"plain UID\"");
    assert(jsonString("\"\\\n\t\r") == "\"\\\"\\\\\\u000a\\u0009\\u000d\"");
    assert(jsonString(std::string("a\0b", 3)) == "\"a\\u0000b\"");
    assert(jsonString("Loa Việt") == "\"Loa Việt\"");
    for (unsigned char i = 0; i < 32; ++i) {
        auto text = jsonString(std::string(1, char(i)));
        assert(text.size() == 8 && text.substr(0, 3) == "\"\\u" && text.back() == '"');
    }
    std::cout << "device JSON: quoting/controls/NUL/UTF-8 passed; no HAL calls\n";
}
