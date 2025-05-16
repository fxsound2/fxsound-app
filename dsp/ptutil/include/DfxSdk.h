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
#ifndef _DFX_SDK_H_
#define _DFX_SDK_H_

// BEGIN TIME TRIAL CODE
#include "dynamic_build_defines.h"
// END TIME TRIAL CODE

/* Boolean values defines */
#define DFX_SDK_TRUE     1
#define DFX_SDK_FALSE    0

/* DFX SDK function return values */
#define DFX_SDK_OKAY     0
#define DFX_SDK_NOT_OKAY 1

/* DFX Handle Definition */
typedef int DFX_SDK_HANDLE;

/* 
 * The different types of knobs 
 */
#define DFX_UI_KNOB_NONE            0
#define DFX_UI_KNOB_FIDELITY        1
#define DFX_UI_KNOB_SURROUND        2
#define DFX_UI_KNOB_AMBIENCE        4
#define DFX_UI_KNOB_DYNAMIC_BOOST   5 
#define DFX_UI_KNOB_BASS_BOOST      6
#define DFX_UI_KNOB_VOCAL_REDUCTION 7

/* HACK */
#define DFX_UI_KNOB_VIDEO_PROCESS_AUDIO 8
#define DFX_UI_KNOB_VIDEO_PROCESS_VIDEO 9

/* The different types of buttons - DFX UI */
#define DFX_UI_BUTTON_BYPASS                 12
#define DFX_UI_BUTTON_FIDELITY               20
#define DFX_UI_BUTTON_AMBIENCE               21
#define DFX_UI_BUTTON_SURROUND               22
#define DFX_UI_BUTTON_DYNAMIC_BOOST          23   
#define DFX_UI_BUTTON_BASS_BOOST             24 
#define DFX_UI_BUTTON_HEADPHONE              25 
#define DFX_UI_BUTTON_MUSIC_MODE             26
#define DFX_UI_BUTTON_VOCAL_REDUCTION_ON     27 
#define DFX_UI_BUTTON_VOCAL_REDUCTION_MODE   28 

/* HACK */
#define DFX_UI_BUTTON_VIDEO_PROCESS_AUDIO    29
#define DFX_UI_BUTTON_VIDEO_PROCESS_VIDEO    30

/* The different types of buttons - Remix UI */
#define DFX_UI_BUTTON_REMIX_BYPASS           100

/* 
 * The different modes for novox processing 
 */
#define DFX_UI_VOCAL_REDUCTION_MODE_1       1
#define DFX_UI_VOCAL_REDUCTION_MODE_2       2

/* 
 * The different modes for music processing 
 */
#define DFX_UI_MUSIC_MODE_MUSIC1      1
#define DFX_UI_MUSIC_MODE_MUSIC2      2
#define DFX_UI_MUSIC_MODE_SPEECH      3

/* 
 * Button values.
 */
#define DFX_UI_OFF_VALUE 0
#define DFX_UI_ON_VALUE  1

/* 
 * Min and max values of knob settings
 */
#define DFX_UI_MIN_VALUE						     0.0
#define DFX_UI_MAX_VALUE							  1.0
/* BEGINE TIME TRIAL CODE */
#ifdef TIME_TRIAL_VERSION
#define DFX_UI_MAX_TRIAL_VALUE					  1.0
#define DFX_UI_MAX_TRIAL_VALUE_DETECT_CHEATING 1.0
#else
#define DFX_UI_MAX_TRIAL_VALUE					  0.55
#define DFX_UI_MAX_TRIAL_VALUE_DETECT_CHEATING 0.56
#endif
/* END TIME TIRAL CODE */

/*
 * Warping vals for mobile presets and mobile preset defines.
 */
#define DFX_MUSIC_MODE2_AMBIENCE_FACTOR      (float)0.34
#define DFX_MUSIC_MODE2_DYNAMIC_BOOST_FACTOR (float)1.8
#define DFX_SPEECH_MODE_DYNAMIC_BOOST_FACTOR (float)1.8
#define DFX_SPEECH_MODE_BASS_BOOST_FACTOR    (float)0.25
#define DFX_SPEECH_MODE_FIDELITY_FACTOR      (float)1.6
#define DFX_NUM_TRIAL_PRESETS 10
#define DFX_NUM_PRO_PRESETS   46

/*
 * Factory preset names
 */
#define DFX_FACTORY_PRESET_1 "No processing"
#define DFX_FACTORY_PRESET_2 "MP3 Enhancer - Standard"
#define DFX_FACTORY_PRESET_3 "MP3 Enhancer - Hi Volume"
#define DFX_FACTORY_PRESET_4 "MP3 Enhancer - No Ambience"
#define DFX_FACTORY_PRESET_5 "Streamcast - High Bit Rate Music"
#define DFX_FACTORY_PRESET_6 "Streamcast - Low Bit Rate Music"
#define DFX_FACTORY_PRESET_7 "Streamcast - Dialog"
#define DFX_FACTORY_PRESET_8 "Large Ambience"
#define DFX_FACTORY_PRESET_9 "Very Large Ambience"
#define DFX_FACTORY_PRESET_10 "Concert Hall Ambience"

/* Registation modes */
#define DFX_REG_UNREGISTERED           1
#define DFX_REG_VERIFIED               2
#define DFX_REG_NOT_YET_VERIFIED       3
#define DFX_REG_TOO_MANY_REGISTRATIONS 4
#define DFX_REG_ILLEGAL_SERIAL         5
#define DFX_REG_OLD_STYLE_SERIAL       6
#define DFX_REG_NO_ADMIN_PRIVILEDGES   7

/* Registration message max length */
#define DFX_MESSAGE_MAX_LENGTH         1024

/* Functions */

#ifndef __ANDROID__
#ifdef __cplusplus
extern "C" {
#endif
int dfxgInit(PT_HANDLE **, HWND, HINSTANCE, int, int, int, wchar_t *, wchar_t *, int, int, int, int, int, int, long, int, long, int, PT_HANDLE *, int *);
int dfxgGetDfxHwnd(PT_HANDLE *, HWND *);
int dfxgGetDfxpHandle(PT_HANDLE *, PT_HANDLE **);
int dfxgSetParentWindow(PT_HANDLE *, HWND);
int dfxgSetButtonValue(PT_HANDLE *, int, int);
int dfxgGetPlaybackStopped(PT_HANDLE *, int *);
int dfxgQuit(PT_HANDLE **);

int dfxpBeginProcess(PT_HANDLE *, int, int, int);
int dfxpModifyShortIntSamples(PT_HANDLE *, short int *, short int *, int);

int dfxgHideMainWindow(PT_HANDLE *);
int dfxgShowMainWindow(PT_HANDLE *);
int dfxgRestoreMainWindow(PT_HANDLE *, int);
int dfxgMinimizeMainWindow(PT_HANDLE *, int, int);

int dfxgRemixCommunicateCheckForChanges(PT_HANDLE *);

int dfxgBringUpRegisterDlgIfNotRegistered(PT_HANDLE *);

#ifdef __cplusplus
}
#endif
#endif //WIN32

#endif //_DFX_SDK_H
