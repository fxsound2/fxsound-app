#ifndef FXSOUND_DSP_WORD_H
#define FXSOUND_DSP_WORD_H
#include <stdint.h>
#include <string.h>
#ifdef _MSC_VER
#define DSP_INLINE static __inline
#else
#define DSP_INLINE static inline
#endif
#if defined(__clang__) || defined(__GNUC__)
#define DSP_ALIAS __attribute__((may_alias))
#else
#define DSP_ALIAS
#endif
#if defined(_WIN32)
typedef long DSP_WORD;
typedef unsigned long DSP_UWORD;
#else
typedef int32_t DSP_WORD;
typedef uint32_t DSP_UWORD;
#endif
#ifdef __cplusplus
static_assert(sizeof(DSP_WORD) == sizeof(float), "DSP transport requires 32-bit words");
#endif
DSP_INLINE DSP_WORD dspFloatBits(float value) {
    DSP_WORD bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
DSP_INLINE float dspBitsFloat(DSP_WORD bits) {
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
DSP_INLINE DSP_WORD dspSignedBits(DSP_UWORD bits) {
    DSP_WORD value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
DSP_INLINE DSP_WORD dspReadWord(const float *values, int index) {
    return dspFloatBits(values[index]);
}
DSP_INLINE void dspWriteWord(float *values, int index, DSP_WORD bits) {
    values[index] = dspBitsFloat(bits);
}
#endif
