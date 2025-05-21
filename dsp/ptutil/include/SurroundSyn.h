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
#ifndef _SURROUND_SYN_H_
#define _SURROUND_SYN_H_

#include "codedefs.h"

// SurroundSynInit.cpp
int PT_DECLSPEC SurroundSynNew(PT_HANDLE **hpp_SurroundSyn);
int PT_DECLSPEC SurroundSynFreeUp(PT_HANDLE **hpp_SurroundSyn);

// SurroundSynProcess.cpp
int PT_DECLSPEC SurroundSynProcess(PT_HANDLE *hp_SurroundSyn,
							/* Input Signal Info */
							realtype *rp_signal_in,		/* Input signal, points interleaved in stereo case */
							realtype *rp_signal_out,	/* Output signal, points interleaved in MS format */
							int i_num_sample_sets,  /* Number of mono sample points or stereo sample input pairs */
							int i_num_channels,     /* 1 for mono, 2 for stereo */
                     realtype r_samp_freq,   /* Sampling frequency in hz. */
							int i_processing_on     /* Flag stating if synthesis processing is on */
					 	);

#endif /* _SURROUND_SYN_H_*/
