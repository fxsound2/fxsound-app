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
#ifndef _C_AURAL_H_
#define _C_AURAL_H_

/* Special structure used for parameters and state of algorithm */
struct dspAuralStructType
{
	/* Parameters common to all dsp functions */
	/* Note- must occupy same 32 word locations as defines in Boardrv1.h */
	long pc_to_dsp_flags;
	long dsp_to_pc_flags;
	long dsp_number_of_elements;
	realtype dsp_sampling_freq;
	long stereo_in_flag;
	long dsp_mute_in_flag;
	long unassigned6;
	long unassigned7;
	long unassigned8;
	long unassigned9;
	realtype dry_gain;
	realtype wet_gain;
	realtype master_gain;
	long dsp_dma_in_transfer;
	long unassigned14;
	long unassigned15;
	long unassigned16;
	long unassigned17;
	long unassigned18;
	long unassigned19;

	/* Note- algorithm specific parameters must occupy same 32 word locations
	 * as defines below.
	 */
	realtype aural_drive;
	realtype aural_odd;
	realtype aural_even;
	realtype gain;
	realtype a1;
	realtype a0;

	/* Algorithm state variables */
	realtype out1_minus1;
	realtype out1_minus2;
	realtype in1_minus1;
	realtype in1_minus2;
	realtype out2_minus1;
	realtype out2_minus2;
	realtype in2_minus1;
	realtype in2_minus2;
};

/* Constants */
#define DSP_AURAL_DRIVE_MIN_VALUE 0.0
#define DSP_AURAL_DRIVE_MAX_VALUE (realtype)(TWO_PI/4.0 * 1.8 * 2.0 * 0.75)

#define DSP_AURAL_WET_BOOST (2.0 * 0.75)
#define DSP_AURAL_EVEN_MIN_VALUE (0.5 * DSP_AURAL_WET_BOOST)
#define DSP_AURAL_EVEN_MAX_VALUE 0.0
#define DSP_AURAL_ODD_MIN_VALUE 0.0
#define DSP_AURAL_ODD_MAX_VALUE (1.0 * DSP_AURAL_WET_BOOST)

/* Algorithm specific parameters */
#define AURAL_DRIVE          20L + COMM_MEM_OFFSET
#define AURAL_ODD            21L + COMM_MEM_OFFSET
#define AURAL_EVEN           22L + COMM_MEM_OFFSET
#define AURAL_FILTER_GAIN    23L + COMM_MEM_OFFSET
#define AURAL_FILTER_A1      24L + COMM_MEM_OFFSET
#define AURAL_FILTER_A0      25L + COMM_MEM_OFFSET

/* Note - if adding more parameters, check in c_play.h .
 * Since the play program shares memory space with the first
 * effect which is the activator, play uses the parameter space
 * above the first effect, the activator. Make sure not to step
 * on those parameters
 */

#endif /* _C_AURAL_H_ */
