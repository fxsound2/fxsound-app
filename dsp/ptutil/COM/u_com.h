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
#ifndef _U_COM_H_
#define _U_COM_H_

#include "codedefs.h"
#include "pt_defs.h"
#include "c_dsps.h"
#include "slout.h"

/* Local Functions */
int com_ReadSerialNum(PT_HANDLE *, int, unsigned long *);

/* com handle definition */
struct comHdlType {
	CSlout *slout_hdl;
	CSlout *debug_slout_hdl;
	char msg1[1024]; /* String for messages */

	/* Card status */
	int card_exists;   /* Flag saying if the main card exists at board address */
	int dongle_exists; /* Flag saying that a dongle is present */
	int cracked_flag;  /* Flag saying if we have been cracked */
	int aes_exists;    /* Flag saying if aes expansion is on card */
	unsigned long serial_num; /* Serial number of card */
	long main_num_samples;    /* Number of samples of memory on main card */
	long expanded_num_samples; /* Number of samples of memory on expansion */
	
	int program_loaded; /* Flag saying if a program has been loaded */
	int program_running; /* Flag saying if a program is running */
	int processor_index; /* This is (processor_num - 1). Always runs from
						  * 0 to 7 consectutively
						  */
	int board_address;   /* Used to reference the correct physical address
						  * for a particular processor number. Note that
						  * the board_address can be any number, depending
						  * on if the user has chosen a non-default physical
						  * address.
						  */
	long base_address;
	char *executable;
	char *arguments; 
	int io_type;      /* PCFG_ANALOG, PCFG_AES, or PCFG_SPDIF */
	int fx_link_flag; /* Flag saying if it is using the fx link */
	int turned_off;   /* Flag saying if com is turned off for development */
   int debug_mode;   /* Flag saying if write values should be printed */ 
	long buffer_size; /* Size of buffers for WAV and DAW processing */
	int softdsp_mode; /* 1 -> software DSP, 0 -> hardware DSP */
	PT_HANDLE *comSftwr_hdl;
};

/* 
 * The following are the memory locations for the serial number and the
 * different password types.  Since the passwords can be stored on either the
 * card or the dongle there are several different defines:
 *
 * 1. COM_PASS_HARD_ONCARD... : The EEPROM location on the hardware card to run
 *                              the specific plug-in in hardware mode.
 * 2. COM_PASS_SOFT_ONCARD... : The EEPROM location on the hardware card to run
 *                              the specific plug-in in soft_dsp mode.
 * 3. COM_PASS_SOFT_DONGLE... : The memory location on the dongle to run the specific
 *                              plug-in in soft_dsp mode.
 */
#define COM_PASS_HARD_ONCARD_SERIAL_NUMBER 0
#define COM_PASS_SOFT_DONGLE_SERIAL_NUMBER 0

#define COM_PASS_HARD_ONCARD_STUDIO_SYSTEM 1
#define COM_PASS_SOFT_ONCARD_STUDIO_SYSTEM 12
#define COM_PASS_SOFT_DONGLE_STUDIO_SYSTEM 2

#define COM_PASS_HARD_ONCARD_REVERB 2
#define COM_PASS_SOFT_ONCARD_REVERB 13
#define COM_PASS_SOFT_DONGLE_REVERB 4

#define COM_PASS_HARD_ONCARD_DELAY 3
#define COM_PASS_SOFT_ONCARD_DELAY 14
#define COM_PASS_SOFT_DONGLE_DELAY 6

#define COM_PASS_HARD_ONCARD_CHORUS 4
#define COM_PASS_SOFT_ONCARD_CHORUS 15
#define COM_PASS_SOFT_DONGLE_CHORUS 8

#define COM_PASS_HARD_ONCARD_FLANGE 5
#define COM_PASS_SOFT_ONCARD_FLANGE 16
#define COM_PASS_SOFT_DONGLE_FLANGE 10
 
#define COM_PASS_HARD_ONCARD_PITCH 6
#define COM_PASS_SOFT_ONCARD_PITCH 17
#define COM_PASS_SOFT_DONGLE_PITCH 12

#define COM_PASS_HARD_ONCARD_PEQ 7
#define COM_PASS_SOFT_ONCARD_PEQ 18
#define COM_PASS_SOFT_DONGLE_PEQ 14
 
#define COM_PASS_HARD_ONCARD_LEADSYN 8
#define COM_PASS_SOFT_ONCARD_LEADSYN 19
#define COM_PASS_SOFT_DONGLE_LEADSYN 16

#define COM_PASS_HARD_ONCARD_PAN 9
#define COM_PASS_SOFT_ONCARD_PAN 20
#define COM_PASS_SOFT_DONGLE_PAN 18

#define COM_PASS_HARD_ONCARD_TREMOLO 10
#define COM_PASS_SOFT_ONCARD_TREMOLO 21
#define COM_PASS_SOFT_DONGLE_TREMOLO 20

#define COM_PASS_HARD_ONCARD_AURAL 11
#define COM_PASS_SOFT_ONCARD_AURAL 22
#define COM_PASS_SOFT_DONGLE_AURAL 22

#define COM_PASS_HARD_ONCARD_MAXIMIZE 12
#define COM_PASS_SOFT_ONCARD_MAXIMIZE 23
#define COM_PASS_SOFT_DONGLE_MAXIMIZE 24

#define COM_PASS_HARD_ONCARD_LEX 13
#define COM_PASS_SOFT_ONCARD_LEX 24
#define COM_PASS_SOFT_DONGLE_LEX 26

#define COM_PASS_HARD_ONCARD_APITCH 14
#define COM_PASS_SOFT_ONCARD_APITCH 25
#define COM_PASS_SOFT_DONGLE_APITCH 28

#define COM_PASS_HARD_ONCARD_CMP 15
#define COM_PASS_SOFT_ONCARD_CMP 26
#define COM_PASS_SOFT_DONGLE_CMP 30

#define COM_PASS_HARD_ONCARD_WID 16
#define COM_PASS_SOFT_ONCARD_WID 27
#define COM_PASS_SOFT_DONGLE_WID 32

#define COM_PASS_HARD_ONCARD_PLY 17
#define COM_PASS_SOFT_ONCARD_PLY 28
#define COM_PASS_SOFT_DONGLE_PLY 34

#define COM_PASS_HARD_ONCARD_GVB 18
#define COM_PASS_SOFT_ONCARD_GVB 29
#define COM_PASS_SOFT_DONGLE_GVB 36

#endif //_U_COM_H
