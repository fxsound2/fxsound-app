/*
FxSound — Linux build

Minimal Linux implementation of the CSlout diagnostic logger used by the DSP.
On Windows this comes from the platform's slout module; here we provide a small
stderr-backed version so the DSP library is self-contained on Linux.
*/

#include "codedefs.h"
#include "slout.h"
#include <cstdio>
#include <cwchar>

int sloutDisplayFileAndLine(char* file, int32_t line)
{
    std::fprintf(stderr, "[dsp] %s:%ld\n", file ? file : "?", line);
    return OKAY;
}

CSlout::CSlout(void)
{
    m_msg[0] = '\0';
    m_wcp_msg[0] = L'\0';
    m_linenum = 0;
}

int CSlout::Display(int, char* msg)  { if (msg) std::fprintf(stderr, "[dsp] %s\n", msg);  return OKAY; }
int CSlout::Message(int, char* msg)  { if (msg) std::fprintf(stderr, "[dsp] %s\n", msg);  return OKAY; }
int CSlout::Error(int, char* msg)    { if (msg) std::fprintf(stderr, "[dsp][error] %s\n", msg); return OKAY; }
int CSlout::Warning(int, char* msg)  { if (msg) std::fprintf(stderr, "[dsp][warn] %s\n", msg);  return OKAY; }

int CSlout::Display_Wide(int, wchar_t* msg) { if (msg) std::fprintf(stderr, "[dsp] %ls\n", msg); return OKAY; }
int CSlout::Message_Wide(int, wchar_t* msg) { if (msg) std::fprintf(stderr, "[dsp] %ls\n", msg); return OKAY; }
int CSlout::Error_Wide(int, wchar_t* msg)   { if (msg) std::fprintf(stderr, "[dsp][error] %ls\n", msg); return OKAY; }
int CSlout::Warning_Wide(int, wchar_t* msg) { if (msg) std::fprintf(stderr, "[dsp][warn] %ls\n", msg); return OKAY; }
