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
#ifndef _MIDI_H_
#define _MIDI_H_  

/* #include <windows.h> */
      
#include "slout.h"

/*
 * These defines are used both to detect the midi command, and to pass
 * back which command was read.
 */ 
#define MIDI_NOTE_OFF         0x80
#define MIDI_NOTE_ON          0x90
#define MIDI_KEY_PRESSURE     0xa0
#define MIDI_CONTROL_CHANGE   0xb0
#define MIDI_PROGRAM_CHANGE   0xc0
#define MIDI_CHANNEL_PRESSURE 0xd0
#define MIDI_PITCH_WHEEL      0xe0

#define MIDI_CHANNEL_MASK 0x0F
#define MIDI_COMMAND_MASK 0xF0   

/* Number of control numbers */
#define MIDI_NUM_CTRL_NUMS 122
#define MIDI_NUM_PHYSICAL_CTRLS 39   

/* Control number of middle C on keyboard */
#define MIDI_MIDDLE_C 60

#define MIDI_MIN_VALUE          0
#define MIDI_ONE_TENTH_VALUE    13
#define MIDI_TWO_TENTHS_VALUE   25
#define MIDI_ONE_FOURTH_VALUE   32
#define MIDI_THREE_TENTHS_VALUE 38
#define MIDI_ONE_THIRD_VALUE    42
#define MIDI_FOUR_TENTHS_VALUE  51
#define MIDI_MID_VALUE          63
#define MIDI_SIX_TENTHS_VALUE   76
#define MIDI_SEVEN_TENTHS_VALUE 89
#define MIDI_EIGHT_TENTHS_VALUE 102
#define MIDI_NINE_TENTHS_VALUE  114
#define MIDI_MAX_VALUE          127

#define MIN_MIDI_COMMAND 128
#define MAX_MIDI_COMMAND 255  

/* Midi defines */
#define MIDI_OFF_MODE       0
#define MIDI_DIRECT_MODE    1
#define MIDI_CATCH_MODE     2
#define MIDI_INCREMENT_MODE 3  

/*
 * This define says how many spin wheel commands to skip before actually
 * processing, because it is so touchy.
 */
#define SPIN_WHEEL_SKIP_FACTOR 10

#define MIDI_CTRL_FADER_SWITCH1     0
#define MIDI_CTRL_FADER_SWITCH2     1
#define MIDI_CTRL_FADER_SWITCH3     2
#define MIDI_CTRL_FADER_SWITCH4     3
#define MIDI_CTRL_FADER_SWITCH5     4
#define MIDI_CTRL_FADER_SWITCH6     5
#define MIDI_CTRL_FADER_SWITCH7     6
#define MIDI_CTRL_FADER_SWITCH8     7
#define MIDI_CTRL_FADER1            8
#define MIDI_CTRL_FADER2            9
#define MIDI_CTRL_FADER3            10
#define MIDI_CTRL_FADER4            11
#define MIDI_CTRL_FADER5            12
#define MIDI_CTRL_FADER6            13
#define MIDI_CTRL_FADER7            14
#define MIDI_CTRL_FADER8            15
#define MIDI_CTRL_FUNCTION_SWITCH0  16
#define MIDI_CTRL_FUNCTION_SWITCH1  17
#define MIDI_CTRL_FUNCTION_SWITCH2  18
#define MIDI_CTRL_FUNCTION_SWITCH3  19
#define MIDI_CTRL_FUNCTION_SWITCH4  20
#define MIDI_CTRL_FUNCTION_SWITCH5  21
#define MIDI_CTRL_FUNCTION_SWITCH6  22
#define MIDI_CTRL_FUNCTION_SWITCH7  23
#define MIDI_CTRL_FUNCTION_SWITCH8  24
#define MIDI_CTRL_FUNCTION_SWITCH9  25
#define MIDI_CTRL_FUNCTION_SWITCH10 26
#define MIDI_CTRL_FUNCTION_SWITCH11 27
#define MIDI_CTRL_FUNCTION_SWITCH12 28
#define MIDI_CTRL_FUNCTION_SWITCH13 29
#define MIDI_CTRL_FUNCTION_SWITCH14 30
#define MIDI_CTRL_MODE_SWITCH       31
#define MIDI_CTRL_KNOB1             32
#define MIDI_CTRL_KNOB2             33
#define MIDI_CTRL_KNOB3             34
#define MIDI_CTRL_KNOB4             35
#define MIDI_CTRL_KNOB5             36
#define MIDI_CTRL_KNOB6             37
#define MIDI_CTRL_SPIN_WHEEL        38              

/* Standard Midi defines */
#define MIDI_STANDARD_MASTER_LEVEL 7 /* WARNING: THIS CLASHES WITH JL-COOPER, SO WE CANNOT USE IT */ 
#define MIDI_STANDARD_MUTE         91
#define MIDI_STANDARD_OMNI_OFF     124
#define MIDI_STANDARD_OMNI_ON      125

 /* midi.c */
int PT_DECLSPEC midiInit(PT_HANDLE **, char *, CSlout *);
int PT_DECLSPEC midiCalcMidiValue(PT_HANDLE *, int, int, int, 
                     int, int *, int *);
int PT_DECLSPEC midiCalcIncrementedMidiVal(PT_HANDLE *, int, int,
							   int, int *);
int PT_DECLSPEC midiFreeUp(PT_HANDLE **);
int PT_DECLSPEC midiDump(PT_HANDLE *); 

/* midiCfg.c */
int PT_DECLSPEC midiGetCtrlNum(PT_HANDLE *, int);  

/* midiVoic.cpp */
int PT_DECLSPEC midiInitVoices(PT_HANDLE *, int);
int PT_DECLSPEC midiAssignToNextVoice(PT_HANDLE *, int, int *, int *);  
int PT_DECLSPEC midiGetVoiceNum(PT_HANDLE *, int, int *, int *);
int PT_DECLSPEC midiTurnOffVoice(PT_HANDLE *, int);

#endif // _MIDI_H_
