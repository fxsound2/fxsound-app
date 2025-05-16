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
#include "codedefs.h"

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "slout.h"
#include "u_spectrum.h"

/*
 * FUNCTION: spectrumReset()
 * DESCRIPTION:
 *   Resets internal state of the spectrum algorithms, to be called when
 *   user selects different song for playback.
 */
int PT_DECLSPEC spectrumReset( PT_HANDLE *hp_spectrum )
{
	struct spectrumHdlType *cast_handle;
	int i;

	cast_handle = (struct spectrumHdlType *)(hp_spectrum);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	// For now only supports a single fixed number of bands
	if( cast_handle->num_bands != SPECTRUM_MAX_NUM_BANDS )
	{
		sprintf(cast_handle->msg1, "Illegal number of bands (%d) in spectrumReset", cast_handle->num_bands);
		(cast_handle->slout_hdl)->Message(FIRST_LINE, cast_handle->msg1);
		return(NOT_OKAY);
	}

	// If the actual signal sampling rate is greater than the max internal rate, the
	// internal rates will be reduced to match rates used in DFX processing.
	if( (cast_handle->samp_freq > (realtype)SPECTRUM_MAXIMUM_SAMP_FREQ) || (cast_handle->samp_freq <= (realtype)0.0) )
		return(NOT_OKAY);

	if( cast_handle->samp_freq > (realtype)SPECTRUM_MAXIMUM_INTERNAL_SAMP_FREQ )
	{
		// 96k and 88.2k cases
		if(cast_handle->samp_freq < (realtype)SPECTRUM_MAXIMUM_SAMP_FREQ)	// 96k and 88.2k cases
		{
			cast_handle->internal_samp_freq = cast_handle->samp_freq/(realtype)2.0;
			cast_handle->internal_rate_ratio = 2;
		}
		else // 192khz case
		{
			cast_handle->internal_samp_freq = cast_handle->samp_freq/(realtype)4.0;
			cast_handle->internal_rate_ratio = 4;
		}
	}
	else	// 48khz and lower cases
	{
		cast_handle->internal_samp_freq = cast_handle->samp_freq;
		cast_handle->internal_rate_ratio = 1;
	}
	cast_handle->internal_samp_period = (realtype)1.0/(cast_handle->internal_samp_freq);

	cast_handle->time_since_last_buffer_store = cast_handle->refresh_rate_secs;

	// Needed to update internal delay variable
	if( spectrumSetDelay((PT_HANDLE *)cast_handle, cast_handle->delay_secs) != OKAY )
		return(NOT_OKAY);

	// Update time constant internal settings based on new samp_freq
	if( spectrumSetTimeConstant((PT_HANDLE *)cast_handle, (realtype)SPECTRUM_DEFAULT_TIME_CONSTANT) != OKAY)
		return(NOT_OKAY);

	// Zero internal states
	cast_handle->in_1 = (realtype)0.0;
	cast_handle->in_2 = (realtype)0.0;

	for(i=0; i<cast_handle->num_bands; i++)
	{
		cast_handle->band_values[i] = (realtype)0.0;
		spectrum_ResetFilter( &(cast_handle->sFilt[i]) );
	}

	for(i=0; i<(SPECTRUM_MAX_NUM_BANDS * (int)SPECTRUM_MAX_BAND_BUFFER_LEN); i++)
		cast_handle->band_buf[i] = (realtype)0.0;

	// Bands are logarithmically spaced from 100 hz. to 10,000 hz.
	// ORIGINAL 5 BAND LOCATIONS
	//      100,      316.228,      1000,      3162.28,      10,000
	//
	// Make -3db points at:
	// 56.23     177.82      562.34     1778.28       5623.42      17782.79
	//
	//
	// NEW 10 BAND LOCATIONS
	//      56.23     100     177.83     316.228     562.34     1000     1778.28     3162.28     5623.4     10,000
	//
	//  10 band -3db points
	//  42.17    74.99  133.35     237.14      421.70     749.89    1333.52    2371.37     4216.97    7498.94     13335.21
	//
	// See design results below.

	// First band
	cast_handle->sFilt[0].a1 = (realtype)1.9952707978;
	cast_handle->sFilt[0].a2 = (realtype)-0.9953348411;
	cast_handle->sFilt[0].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_1_WARP)/((realtype)cast_handle->num_channels * (realtype)4.245657595e+02);
	
	// Second band
	cast_handle->sFilt[1].a1 = (realtype)1.9915173377;
	cast_handle->sFilt[1].a2 = (realtype)-0.9917194870;
	cast_handle->sFilt[1].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_2_WARP)/((realtype)cast_handle->num_channels * (realtype)2.391965397e+02);
	
	// Third band
	cast_handle->sFilt[2].a1 = (realtype)1.9846835136;
	cast_handle->sFilt[2].a2 = (realtype)-0.9853206989;
	cast_handle->sFilt[2].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_3_WARP)/((realtype)cast_handle->num_channels * (realtype)1.349296378e+02);

	// Fourth band
	cast_handle->sFilt[3].a1 = (realtype)1.9720410075;
	cast_handle->sFilt[3].a2 = (realtype)-0.9740444157;
	cast_handle->sFilt[3].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_4_WARP)/((realtype)cast_handle->num_channels * (realtype)7.631087758e+01);

	// Fifth band
	cast_handle->sFilt[4].a1 = (realtype)1.9480305935;
	cast_handle->sFilt[4].a2 = (realtype)-0.9543009461;
	cast_handle->sFilt[4].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_5_WARP)/((realtype)cast_handle->num_channels * (realtype)4.334342216e+01);

	// Sixth band
	cast_handle->sFilt[5].a1 = (realtype)1.9006550741;
	cast_handle->sFilt[5].a2 = (realtype)-0.9201218454;
	cast_handle->sFilt[5].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_6_WARP)/((realtype)cast_handle->num_channels * (realtype)2.479951362e+01);

	// Seventh band
	cast_handle->sFilt[6].a1 = (realtype)1.8025225345;
	cast_handle->sFilt[6].a2 = (realtype)-0.8620772515;
	cast_handle->sFilt[6].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_7_WARP)/((realtype)cast_handle->num_channels * (realtype)1.436694455e+01);

	// Eighth band
	cast_handle->sFilt[7].a1 = (realtype)1.5891186613;
	cast_handle->sFilt[7].a2 = (realtype)-0.7664106181;
	cast_handle->sFilt[7].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_8_WARP)/((realtype)cast_handle->num_channels * (realtype)8.490790030e+00);

	// Nineth band
	cast_handle->sFilt[8].a1 = (realtype)1.1149497494;
	cast_handle->sFilt[8].a2 = (realtype)-0.6153052550;
	cast_handle->sFilt[8].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_9_WARP)/((realtype)cast_handle->num_channels * (realtype)5.169741233e+00);

	// Tenth band
	cast_handle->sFilt[9].a1 = (realtype)0.1311997923;
	cast_handle->sFilt[9].a2 = (realtype)-0.3874425954;
	cast_handle->sFilt[9].gain = (cast_handle->sensitivity * (realtype)SPECTRUM_BAND_10_WARP)/((realtype)cast_handle->num_channels * (realtype)3.264452631e+00);

	return(OKAY);
}

/* FILTER DESIGN RESULTS  10 BAND DESIGNS */
// http://www-users.cs.york.ac.uk/~fisher/mkfilter/
/*
Filter 1

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 9.5623582766e-04 1.7004535147e-03
raw alpha1    =   0.0009562358
raw alpha2    =   0.0017004535
warped alpha1 =   0.0009562387
warped alpha2 =   0.0017004697
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 4.245657595e+02   phase =  -0.0442948691 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0023380706 + j   0.0076633871
	 -0.0023380706 + j  -0.0076633871

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9976353989 + j   0.0076453889
	  0.9976353989 + j  -0.0076453889

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9953348411 * y[n- 2])
     + (  1.9952707978 * y[n- 1])


Filter 2

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 1.7004535147e-03 3.0238095238e-03
raw alpha1    =   0.0017004535
raw alpha2    =   0.0030238095
warped alpha1 =   0.0017004697
warped alpha2 =   0.0030239005
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 2.391965397e+02   phase =  -0.0442910466 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0041576805 + j   0.0136276827
	 -0.0041576805 + j  -0.0136276827

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9957586689 + j   0.0135705719
	  0.9957586689 + j  -0.0135705719

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9917194870 * y[n- 2])
     + (  1.9915173377 * y[n- 1])


Filter 3

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 3.0238095238e-03 5.3773242630e-03
raw alpha1    =   0.0030238095
raw alpha2    =   0.0053773243
warped alpha1 =   0.0030239005
warped alpha2 =   0.0053778359
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 1.349296378e+02   phase =  -0.0442879040 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0073951061 + j   0.0242345215
	 -0.0073951061 + j  -0.0242345215

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9923417568 + j   0.0240527881
	  0.9923417568 + j  -0.0240527881

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9853206989 * y[n- 2])
     + (  1.9846835136 * y[n- 1])


Filter 4

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 5.3773242630e-03 9.5623582766e-03
raw alpha1    =   0.0053773243
raw alpha2    =   0.0095623583
warped alpha1 =   0.0053778359
warped alpha2 =   0.0095652359
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 7.631087758e+01   phase =  -0.0442642215 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0131551051 + j   0.0431013180
	 -0.0131551051 + j  -0.0431013180

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9860205037 + j   0.0425203706
	  0.9860205037 + j  -0.0425203706

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9740444157 * y[n- 2])
     + (  1.9720410075 * y[n- 1])


Filter 5

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 9.5623582766e-03 1.7004308390e-02
raw alpha1    =   0.0095623583
raw alpha2    =   0.0170043084
warped alpha1 =   0.0095652359
warped alpha2 =   0.0170205023
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 4.334342216e+01   phase =  -0.0441953132 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0234214102 + j   0.0766728506
	 -0.0234214102 + j  -0.0766728506

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9740152968 + j   0.0748007208
	  0.9740152968 + j  -0.0748007208

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9543009461 * y[n- 2])
     + (  1.9480305935 * y[n- 1])


Filter 6

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 1.7004308390e-02 3.0238548753e-02
raw alpha1    =   0.0170043084
raw alpha2    =   0.0302385488
warped alpha1 =   0.0170205023
warped alpha2 =   0.0303298406
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 2.479951362e+01   phase =  -0.0439809021 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0418125194 + j   0.1364976998
	 -0.0418125194 + j  -0.1364976998

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9503275371 + j   0.1303818152
	  0.9503275371 + j  -0.1303818152

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9201218454 * y[n- 2])
     + (  1.9006550741 * y[n- 1])


Filter 7

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 3.0238548753e-02 5.3772562358e-02
raw alpha1    =   0.0302385488
raw alpha2    =   0.0537725624
warped alpha1 =   0.0303298406
warped alpha2 =   0.0542899874
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 1.436694455e+01   phase =  -0.0432948522 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0752730211 + j   0.2435967934
	 -0.0752730211 + j  -0.2435967934

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9012612672 + j   0.2231711892
	  0.9012612672 + j  -0.2231711892

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.8620772515 * y[n- 2])
     + (  1.8025225345 * y[n- 1])


Filter 8

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 5.3772562358e-02 9.5622902494e-02
raw alpha1    =   0.0537725624
raw alpha2    =   0.0956229025
warped alpha1 =   0.0542899874
warped alpha2 =   0.0986071745
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 8.490790030e+00   phase =  -0.0410912484 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.1392265496 + j   0.4381312912
	 -0.1392265496 + j  -0.4381312912

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.7945593307 + j   0.3675405939
	  0.7945593307 + j  -0.3675405939

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.7664106181 * y[n- 2])
     + (  1.5891186613 * y[n- 1])


Filter 9

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 9.5622902494e-02 1.7004399093e-01
raw alpha1    =   0.0956229025
raw alpha2    =   0.1700439909
warped alpha1 =   0.0986071745
warped alpha2 =   0.1883073236
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 5.169741233e+00   phase =  -0.0337442047 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.2818013294 + j   0.8084807713
	 -0.2818013294 + j  -0.8084807713

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.5574748747 + j   0.5518396679
	  0.5574748747 + j  -0.5518396679

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.6153052550 * y[n- 2])
     + (  1.1149497494 * y[n- 1])


Filter 10

Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 1.7004399093e-01 3.0238571429e-01
raw alpha1    =   0.1700439909
raw alpha2    =   0.3023857143
warped alpha1 =   0.1883073236
warped alpha2 =   0.4450933574
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 3.264452631e+00   phase =  -0.0058293122 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.8067171174 + j   1.6303573120
	 -0.8067171174 + j  -1.6303573120

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.0655998961 + j   0.6189824303
	  0.0655998961 + j  -0.6189824303

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.3874425954 * y[n- 2])
     + (  0.1311997923 * y[n- 1])

//
//
/* FILTER DESIGN RESULTS  ORIGINAL 5 BAND DESIGNS */

/*
Filter 1

You specified the following parameters: 
filtertype  =  Butterworth  
passtype  =  Bandpass  
ripple  =   
order  =  1  
samplerate  =  44100  
corner1  =  56.23  
corner2  =  177.82  
adzero  =   
logmin  =  -40  

Results 
Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 1.2750566893e-03 4.0321995465e-03
raw alpha1    =   0.0012750567
raw alpha2    =   0.0040321995
warped alpha1 =   0.0012750635
warped alpha2 =   0.0040324152
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 1.127067589e+02   phase =  -0.0808872841 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0086624759 + j   0.0113111967
	 -0.0086624759 + j  -0.0113111967

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9913117360 + j   0.0112134911
	  0.9913117360 + j  -0.0112134911

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9828247003 * y[n- 2])
     + (  1.9826234720 * y[n- 1])


Filter 2

You specified the following parameters: 
filtertype  =  Butterworth  
passtype  =  Bandpass  
ripple  =   
order  =  1  
samplerate  =  44100  
corner1  =  177.82  
corner2  =  562.34  
adzero  =   
logmin  =  -40  

Results 
Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 4.0321995465e-03 1.2751473923e-02
raw alpha1    =   0.0040321995
raw alpha2    =   0.0127514739
warped alpha1 =   0.0040324152
warped alpha2 =   0.0127582995
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 3.629491361e+01   phase =  -0.0808277516 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0274131739 + j   0.0357708628
	 -0.0274131739 + j  -0.0357708628

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9723435030 + j   0.0347992357
	  0.9723435030 + j  -0.0347992357

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.9466628746 * y[n- 2])
     + (  1.9446870060 * y[n- 1])


Filter 3

You specified the following parameters: 
filtertype  =  Butterworth  
passtype  =  Bandpass  
ripple  =   
order  =  1  
samplerate  =  44100  
corner1  =  562.34  
corner2  =  1778.28  
adzero  =   
logmin  =  -40  

Results 
Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 1.2751473923e-02 4.0323809524e-02
raw alpha1    =   0.0127514739
raw alpha2    =   0.0403238095
warped alpha1 =   0.0127582995
warped alpha2 =   0.0405409097
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 1.212028356e+01   phase =  -0.0802210600 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.0872816442 + j   0.1131435267
	 -0.0872816442 + j  -0.1131435267

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.9107537369 + j   0.1035746264
	  0.9107537369 + j  -0.1035746264

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.8402000725 * y[n- 2])
     + (  1.8215074737 * y[n- 1])


Filter 4
You specified the following parameters: 
filtertype  =  Butterworth  
passtype  =  Bandpass  
ripple  =   
order  =  1  
samplerate  =  44100  
corner1  =  1778.28  
corner2  =  5623.42  
adzero  =   
logmin  =  -40  

Results 
Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 4.0323809524e-02 1.2751519274e-01
raw alpha1    =   0.0403238095
raw alpha2    =   0.1275151927
warped alpha1 =   0.0405409097
warped alpha2 =   0.1348047415
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 4.436354472e+00   phase =  -0.0739818747 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -0.2961385615 + j   0.3578487432
	 -0.2961385615 + j  -0.3578487432

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	  0.7007463681 + j   0.2650580242
	  0.7007463681 + j  -0.2650580242

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + ( -0.5613012286 * y[n- 2])
     + (  1.4014927362 * y[n- 1])


Filter 5
You specified the following parameters: 
filtertype  =  Butterworth  
passtype  =  Bandpass  
ripple  =   
order  =  1  
samplerate  =  44100  
corner1  =  5623.42  
corner2  =  17782.79  
adzero  =   
logmin  =  -40  

Results 
Command line: /www/usr/fisher/helpers/mkfilter -Bu -Bp -o 1 -a 1.2751519274e-01 4.0323786848e-01
raw alpha1    =   0.1275151927
raw alpha2    =   0.4032378685
warped alpha1 =   0.1348047415
warped alpha2 =   1.0146615664
gain at dc    :   mag = 0.000000000e+00
gain at centre:   mag = 1.848306928e+00   phase =   0.0142545795 pi
gain at hf    :   mag = 0.000000000e+00

S-plane zeros:
	  0.0000000000 + j   0.0000000000

S-plane poles:
	 -4.2610251050 + j   0.0000000000
	 -1.2672783694 + j   0.0000000000

Z-plane zeros:
	  1.0000000000 + j   0.0000000000
	 -1.0000000000 + j   0.0000000000

Z-plane poles:
	 -0.3611269827 + j   0.0000000000
	  0.2242605459 + j   0.0000000000

Recurrence relation:
y[n] = ( -1 * x[n- 2])
     + (  0 * x[n- 1])
     + (  1 * x[n- 0])

     + (  0.0809865343 * y[n- 2])
     + ( -0.1368664369 * y[n- 1])
*/