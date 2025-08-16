/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)

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

/*
 * FILE: u_sos.h
 * DATE: 7/2/96
 * AUTOHOR: Mark Kaplan
 * DESCRIPTION:
 *
 * Local header file for the sos coefficients module
 * This section type is of the normalized form
 *
 *                b0*Z**2 + b1*Z + b2
 *    H(Z) = g* ----------------------
 *                 Z**2 + a1*Z + a2
 */

#ifndef _U_SOS_H_
#define _U_SOS_H_

#include "slout.h"

#define SOS_FLOAT_BIAS 1.0e-30 // Was 1.0e-5 Thru 7/11/15, then changed to 1.0e-30 values that was being used in Hyperbass.
#define SOS_DCBLOCK_ALPHA 0.999	// See http://peabody.sapp.org/class/dmp2/lab/dcblock/ for freq response curves for differen values, 0.999 is good for 44.1 and 48khz.

/* Section type definition */
struct sosSectionType {
   realtype b0;
   realtype b1;
   realtype b2;
   realtype a1;
   realtype a2;
   realtype a1_old;
   realtype a2_old;
	realtype state1;
	realtype state2;
	realtype state3;
	realtype state4;
	// Added for Surround Sound function
	realtype state_1[8];
	realtype state_2[8];
};   

/* Sos handle definition */
struct sosHdlType {
   /* Initialization info */
   CSlout *slout_hdl;
   char msg1[1024]; /* String for messages */

   int num_allocated_sections;  /* The number of sections allocated */
   int num_active_sections;     /* Active number of sections (must be <= num allocated) */
   int sos_response_valid[SOS_MAX_NUM_SOS_SECTIONS]; /* IS_TRUE if sos response, freq is current */ 
   realtype sos_center_freq[SOS_MAX_NUM_SOS_SECTIONS];/* Contains current center freq of section */
   int sos_center_freq_indexes[SOS_MAX_NUM_SOS_SECTIONS];/* Contains pixel index corresponding to center freq of section */
   realtype sos_center_freq_response[SOS_MAX_NUM_SOS_SECTIONS];/* Contains power response at center freq */
   realtype balance_left;       /* Externally applied balance for the combined sections */
   realtype balance_right;      /* Externally applied balance for the combined sections */
   realtype target_rms;         /* Externally applied normalization rms  for the combined sections */
   realtype normalization_gain; /* normalization gain */
   realtype master_gain; /* Externally applied master gain for the combined sections */
   int sos_type[SOS_MAX_NUM_SOS_SECTIONS]; /* SOS_PARA for parametric, SOS_SHELF for shelf type */
   struct sosSectionType *sections; 
   int section_on_flag[SOS_MAX_NUM_SOS_SECTIONS]; /* If IS_TRUE, then displayed gain is not 0 */

	realtype in1_old, in2_old, outDC1_old, outDC2_old;	// Added for DC blocking filter.
	realtype in1_oldSS[8];
	realtype in2_oldSS[8];
	realtype outDC1_oldSS[8];
	realtype outDC2_oldSS[8];

};

#endif //_U_SOS_H_
