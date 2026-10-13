#include "codedefs.h"
#include "pt_defs.h"
#include "c_aural.h"
#include "c_play.h"
#include "c_lex.h"
#include "c_max.h"
#include "c_wid.h"
#include <cstddef>
static_assert(sizeof(float) == 4);
static_assert(sizeof(DSP_WORD) == 4 && sizeof(DSP_UWORD) == 4);
static_assert(sizeof(hardwareMeterValType) == 56);
static_assert(offsetof(dspAuralStructType, stereo_in_flag) == 16);
static_assert(offsetof(dspPlayStructType, stereo_in_flag) == 16);
static_assert(offsetof(dspLexStructType, stereo_in_flag) == 16);
static_assert(offsetof(dspMaxiStructType, stereo_in_flag) == 16);
static_assert(offsetof(dspWideStructType, stereo_in_flag) == 16);
static_assert(offsetof(dspLexStructType, roomsize) == 80);
static_assert(offsetof(dspAuralStructType, dry_gain) == 40);
