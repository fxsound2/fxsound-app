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
#ifndef _PT_DEFS_H_
#define _PT_DEFS_H_

#ifndef __ANDROID__
#include <windows.h>
#endif //WIN32

/* Maximum number of DSP/FX processors */
/* Defined to be 16 in Virtual Pack version 6.2 */
/* #define DSPFX_MAX_NUM_PROCS 16 */
/* Increased to 64 for Cakewalk SDK version */
#define DSPFX_MAX_NUM_PROCS 64

/* The number of different plug-in types */
#define DSPFX_NUM_PLUGIN_TYPES 32

/* Common Definitions */
#define DSP_MAX_SAMP_FREQ 48000.0
#define PT_SAMP_FREQ_48K      8
#define PT_SAMP_FREQ_44_1K    0
#define PT_SAMP_FREQ_32K     72
#define PT_SAMP_FREQ_29_4K   64
#define PT_SAMP_FREQ_24K     40
#define PT_SAMP_FREQ_22_05K  32
#define PT_SAMP_FREQ_19_2K  104
#define PT_SAMP_FREQ_17_64K  96
#define PT_SAMP_FREQ_16K     24
#define PT_SAMP_FREQ_14_7K   16
#define PT_SAMP_FREQ_12K     88
#define PT_SAMP_FREQ_11_025K 80
#define PT_SAMP_FREQ_9_6K    56
#define PT_SAMP_FREQ_8_82K   48
#define PT_SAMP_FREQ_8K     120
#define PT_SAMP_FREQ_7_35K  112
/* Add additional freqs for AES- not supported by Crystal part */
#define PT_SAMP_FREQ_44_056K 113
#define PT_SAMP_FREQ_BAD_VALUE 114
#define PT_SAMP_FREQ_48K_4     115 /* The _4 entries are for 4% error- others are 400 PPM */
#define PT_SAMP_FREQ_44_1K_4   116
#define PT_SAMP_FREQ_44_056K_4 117
#define PT_SAMP_FREQ_32K_4     118

#define PT_SAMP_FREQ_MASK 120

/* Hardware oriented numeric constants */
#define TWO_PI 6.283185307


/* Definition of meter info structure */
struct hardwareMeterValType
{
	int  values_are_new;
	long left_in;
	long right_in;
	long left_out;
	long right_out;
	long dsp_status;
	float aux_vals[8]; /* For addition info transfer */
};

/* Max address of eeprom */
#define COM_EEPROM_MAX_MEMORY 63

/* Maximum fullpath string length */
#define PT_MAX_PATH_STRLEN             1024
#define PT_MAX_GENERIC_STRLEN          512
#define PT_MAX_URL_STRLEN		         2048
#define PT_MAX_MESSAGE_STRLEN          2048
#define PT_MAX_SQLITE_STATEMENT_STRLEN 2048
#define PT_MAX_COMMAND_LINE_STRLEN		2048

#endif /* _PT_DEFS_H */    
