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

//#include "pt_versions.h"

/* The different effect names displayed in title bar */
#define DSPFX_EFFECT1_NAME  "AcousticVerb"
#define DSPFX_EFFECT2_NAME  "Delay"
#define DSPFX_EFFECT3_NAME  "Chorus"
#define DSPFX_EFFECT4_NAME  "Flange"
#define DSPFX_EFFECT5_NAME  "Pitch Shift"
#define DSPFX_EFFECT6_NAME  "Parametric EQ"
#define DSPFX_EFFECT8_NAME  "Auto-Pan"
#define DSPFX_EFFECT9_NAME  "Tremolo"
#define DSPFX_EFFECT10_NAME "Compressor"
#define DSPFX_EFFECT11_NAME "PerfectTune"
#define DSPFX_EFFECT12_NAME "Aural Activator"
#define DSPFX_EFFECT13_NAME "Optimizer"
#define DSPFX_EFFECT14_NAME "StudioVerb"
#define DSPFX_EFFECT15_NAME "Stereo Widener"
#define DSPFX_EFFECT16_NAME "DFX"
#define DSPFX_EFFECT17_NAME "GameVerb"

/* Maximum number of DSP/FX processors */
/* Defined to be 16 in Virtual Pack version 6.2 */
/* #define DSPFX_MAX_NUM_PROCS 16 */
/* Increased to 64 for Cakewalk SDK version */
#define DSPFX_MAX_NUM_PROCS 64

/* The number of different plug-in types */
#define DSPFX_NUM_PLUGIN_TYPES 32

/* Maximum size in bytes for a preset (used by SAW) */
#define DSPFX_MAX_PRESET_SIZE 4096

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

/* Defines for data exchange and input and output metering */
#define OUT1_CLIP      1
#define OUT2_CLIP      2
#define IN1_CLIP       4
#define IN2_CLIP       8
#define AES_READ_ERROR 16
#define AES_GOT_SIGNAL 32

/* Defines for PC status flag writes to DSP */
#define PC_GOT_AES_READ_ERROR 1
#define PC_GOT_AES_GOT_SIGNAL 2

#define PC_WRITE_STATE_MIN 0 /* Minimum in state for reading from dsp */
#define PC_WRITE_STATE_MAX 4 /* Number of values in metering series */
#define NO_IN_STATE    -1
#define LEFT_IN_STATE   0      /* First one should always start at 0 */
#define RIGHT_IN_STATE  1
#define LEFT_OUT_STATE  2
#define RIGHT_OUT_STATE 3
#define STATUS_STATE    4

/* Buffered Input Modes */
#define BUFMODE_NONE                     0 /* Non-buffered mode */
#define BUFMODE_WAV_PROCESS_PLAY_OTHERSC 1 /* Wav process and play on other soundcard */
#define BUFMODE_WAV_PROCESS_PLAY_DIRECT  2 /* Wav process and play out directly out of DSP/FX */
#define BUFMODE_DAW_PLAY_OTHERSC         3 /* Daw process and play on other soundcard */
#define BUFMODE_SC_NOEFFECT              4 /* Soundcard, no effect */
#define BUFMODE_SC_PLAYONLY_EFFECTOUT    5 /* Soundcard, play and effect output */
#define BUFMODE_SC_RECONLY_EFFECTIN      6 /* Soundcard, record and effect input */
#define BUFMODE_SC_PLAYONLY_EFFECTDAW    7 /* Soundcard, play and effect daw track */

/* Max size of soundcard buffers */
#define PT_MAX_SOUNDCARD_BUF_SIZE 64000

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

/* Parameter values as the DSP card expects them */
#define DSP_DRY_MIN_VALUE            1.0
#define DSP_DRY_MAX_VALUE            0.0
#define DSP_WET_MIN_VALUE            0.0
#define DSP_WET_MAX_VALUE            1.0
#define DSP_MASTER_GAIN_MIN_VALUE    0.0
#define DSP_MASTER_GAIN_MAX_VALUE    1.0

#define DSP_PAN_GAIN_LEFT_MIN_VALUE  1.0
#define DSP_PAN_GAIN_LEFT_MID_VALUE  0.5
#define DSP_PAN_GAIN_LEFT_MAX_VALUE  0.0
#define DSP_PAN_GAIN_RIGHT_MIN_VALUE 0.0
#define DSP_PAN_GAIN_RIGHT_MID_VALUE 0.5
#define DSP_PAN_GAIN_RIGHT_MAX_VALUE 1.0
#define DSP_ELEM_GAIN_MIN_VALUE      0.0
#define DSP_ELEM_GAIN_MAX_VALUE      1.0

/* Max address of eeprom */
#define COM_EEPROM_MAX_MEMORY 63

/*
 * The seed for generating hidden locations based on the enterprise
 * ids send to the hide module. 
 */
#define HIDE_DSPFX_ENTERPRISE_RETAIL_SEED 0x37f3ac27
#define HIDE_DSPFX_ENTERPRISE_DEMO_SEED   0x4a36db22
#define HIDE_DFX_ENTERPRISE_RETAIL_SEED   0xb8cee791
#define HIDE_DFX_ENTERPRISE_DEMO_SEED     0x7a86e3f6

/* 
 * The following are the enterprise wide identifiers for the
 * different types of hidden files.
 */
#define HIDDEN_CRACK_TURD                  0
#define HIDDEN_FIRST_RUN_DATE_COPY1_STUDIO 100
#define HIDDEN_FIRST_RUN_DATE_COPY2_STUDIO 101
#define HIDDEN_LAST_RUN_DATE_STUDIO        102
#define HIDDEN_FIRST_RUN_DFX               103
#define HIDDEN_LAST_RUN_DFX                104

/* The following are the different types of products */
#define DSPFX_PRODUCT_STUDIO_PACK    1
#define DSPFX_PRODUCT_DEMO           2
#define DFX_PRODUCT_RETAIL           3
#define DFX_PRODUCT_DEMO             4

/* The following are the website location strings */
#define DSPFX_RETAIL_REG_WEBSITE "http://www.dspfx.com/register.html"
#define DSPFX_DEMO_REG_WEBSITE "http://www.dspfx.com/demoreg.html"
#define DFX_RETAIL_REG_WEBSITE "http://www.fxsound.com/password.html"

/* The following are the top registry locations */
#define DFX_RETAIL_REGISTRY_TOP        "SOFTWARE\\DFX"
#define DFX_RETAIL_REGISTRY_TOP_WIDE   L"SOFTWARE\\DFX"

#define DSPFX_RETAIL_REGISTRY_TOP      "SOFTWARE\\DSPFX Virtual Pack"
#define DSPFX_RETAIL_REGISTRY_TOP_WIDE L"SOFTWARE\\DSPFX Virtual Pack"

#define DSPFX_DEMO_REGISTRY_TOP        "SOFTWARE\\DSPFX Demo"
#define DSPFX_DEMO_REGISTRY_TOP_WIDE   L"SOFTWARE\\DSPFX Demo"

#define DFX_DEMO_REGISTRY_TOP          "SOFTWARE\\DFX Demo"
#define DFX_DEMO_REGISTRY_TOP_WIDE     L"SOFTWARE\\DFX Demo"

/* The following are the contact info locations */
#define DSPFX_POWER_T_NAME          "Power Technology"
#define DSPFX_POWER_T_PHONE         "(415) 467-7886"
#define DSPFX_POWER_T_FAX           "(415) 467-7386"
#define DSPFX_POWER_T_SUPPORT_EMAIL "dsphelp@dspfx.com"
#define DSPFX_POWER_T_SALES_EMAIL   "dspfx@dspfx.com"
#define DSPFX_POWER_T_IREG_EMAIL    "dspfxireg@dspfx.com"
#define DSPFX_POWER_T_WEBSITE       "http://www.dspfx.com"

#define DSPFX_EVENT_NAME          "Event Electronics"
#define DSPFX_EVENT_PHONE         "(805) 566-9993"
#define DSPFX_EVENT_FAX           "(805) 566-7771"
#define DSPFX_EVENT_SUPPORT_EMAIL "info@event1.com"
#define DSPFX_EVENT_WEBSITE       "http://www.event1.com"

/* The different types of products that can be built */
#define PT_PRODUCT_DSPFX 1
#define PT_PRODUCT_DFX   2

/* The different ways to iconize a main window */
#define PT_ICONIZE_UP     1
#define PT_ICONIZE_DOWN   2
#define PT_ICONIZE_BY_CFG 3

/* Extensions (remix specific) */
#define PT_REMIX_PRESET_EXTENSION_WIDE          L"rxp"
#define PT_REMIX_LOOP_EXTENSION_WIDE            L"rxl"
#define PT_REMIX_CONFUSE_CAB_EXT_WIDE           L"rxc"
#define PT_REMIX_CAB_EXT_WIDE                   L"cab"
#define PT_REMIX_SOUND_ENCRYPTED_EXTENSION_WIDE L"rxs"
#define PT_REMIX_SOUND_DECRYPTED_EXTENSION_WIDE L"wav"

/* 
 * Name of the silent sound.  The strlen is set as a define for efficiency 
 */
#define PT_REMIX_SILENT_SOUND_NAME_WIDE     L"Silence"

#define PT_REMIX_SILENT_SOUND_NAME_STRLEN   7

/* Disk Space Calculation Defines */
#define PT_BYTES_PER_KILOBYTE_DISK_SPACE 1024

/* Registry names across products */
#define POWERTECHNOLOGY_REGISTRY_TOP		L"Software\\PowerTechnology"

/************************************************/
/*  LANGUAGE ISO STANDARD ABBREVIATION STRINGS	*/
/************************************************/
#define PT_LANGUAGE_ISO_STRING_GERMAN	L"deu"
#define PT_LANGUAGE_ISO_STRING_ENGLISH	L"eng"

 /* Maximum fullpath string length */
#define PT_MAX_PATH_STRLEN             1024
#define PT_MAX_GENERIC_STRLEN          512
#define PT_MAX_URL_STRLEN		         2048
#define PT_MAX_MESSAGE_STRLEN          2048
#define PT_MAX_SQLITE_STATEMENT_STRLEN 2048
#define PT_MAX_COMMAND_LINE_STRLEN		2048

#endif /* _PT_DEFS_H */    
