#include "slout.h"
#include "portable-runtime.h"
CSlout::CSlout() : m_msg{}, m_wcp_msg{}, m_linenum(0) {}
int CSlout::Display(int, char *message) { return std::fprintf(stderr, "%s\n", message) < 0 ? 1 : 0; }
int CSlout::Message(int line, char *message) { return Display(line, message); }
int CSlout::Error(int line, char *message) { return Display(line, message); }
int CSlout::Warning(int line, char *message) { return Display(line, message); }
int CSlout::Display_Wide(int line, wchar_t *message) {
    auto bytes = dspUtf8(message);
    return Display(line, bytes.data());
}
int CSlout::Message_Wide(int line, wchar_t *message) { return Display_Wide(line, message); }
int CSlout::Error_Wide(int line, wchar_t *message) { return Display_Wide(line, message); }
int CSlout::Warning_Wide(int line, wchar_t *message) { return Display_Wide(line, message); }
