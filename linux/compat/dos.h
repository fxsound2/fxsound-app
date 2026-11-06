/*
FxSound Linux port — compat/dos.h stub (Phase 5)
Scoped to DfxDsp/ptutil only. Legacy DOS header, only needed to parse
Prelst.cpp on Linux. Windows uses real SDK.
*/
#pragma once
#include <stdint.h>
struct dos_date_t { uint8_t day; uint8_t month; uint16_t year; uint8_t dayofweek; };
struct dos_time_t { uint8_t hour; uint8_t minute; uint8_t second; uint8_t hsecond; };
