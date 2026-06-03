/*
FxSound — Linux build

Standalone Linux copy of the two PWAV sample-format converters used by the DSP
processing path. The full PwavConvert.cpp pulls in the Windows multimedia
(mmio/waveOut) handle header u_pwav.h, which is excluded on Linux; these two
functions are self-contained, so they are reproduced here verbatim.
*/

#include "codedefs.h"
#include <cstring>

int PT_DECLSPEC pwav24BitToFloat(char* cp_24_bit, realtype* rp_float,
                                 int i_length, int i_stereo_flag)
{
    int32_t itmp;
    if (i_stereo_flag) i_length *= 2;
    for (int i = 0; i < i_length; i++)
    {
        itmp = 0;
        std::memcpy(&itmp, (cp_24_bit + i * 3), 3);
        itmp <<= 8; /* fills the first 3 lsb's, so shift left */
        rp_float[i] = (float)itmp * ((realtype)1.0 / (realtype)2147483648.0);
    }
    return OKAY;
}

int PT_DECLSPEC pwavFloatTo24Bit(realtype* rp_float, char* cp_24_bit,
                                 int i_length, int i_stereo_flag)
{
    int32_t itmp;
    if (i_stereo_flag) i_length *= 2;
    for (int i = 0; i < i_length; i++)
    {
        realtype ftmp = rp_float[i];
        if (ftmp >= (realtype)1.0)        itmp = 2147483647L;
        else if (ftmp <= (realtype)-1.0)  itmp = -2147483647L;
        else                              itmp = (int32_t)(ftmp * (realtype)2147483648.0);
        itmp >>= 8; /* copies the first 3 lsb's */
        std::memcpy((cp_24_bit + i * 3), &itmp, 3);
    }
    return OKAY;
}
