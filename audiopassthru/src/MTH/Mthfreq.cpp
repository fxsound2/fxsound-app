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

/* Standard includes */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "codedefs.h"

#include "mth.h"

#include "u_mth.h"

/*
 * FUNCTION: mthMidiOctaveFreqs()
 * DESCRIPTION:
 *
 *  Generates a special frequency range that covers 20 hz to 20000 hz
 *  with 122 points. For use specifically with response plotting.
 *  The frequencies occur right on 440 tuning note values,
 *  except for the 20 and 20000 hz endpoint values.
 */
int PT_DECLSPEC mthMidiOctaveFreqs( realtype *freqs, int num_pts ) 
{
/* Define local to this function only */
#define MTH_NUM_PLOT_OCTAVE_PTS 122

    int j, index;
    
  	if( freqs == NULL )
  		return(NOT_OKAY);
  	
    /* Hard set octave values. Choose 440 tuning octave locations to allow
     * good upper and lower endpoints. First point at 20 hz, then to note freqs.
     */
    /* index 6 is 27.5 hz */
    #define QNT_OCTAVE_OFFSET 6
    freqs[QNT_OCTAVE_OFFSET + 0*12] = (realtype)27.5;
    freqs[QNT_OCTAVE_OFFSET + 1*12] = (realtype)55.0;
    freqs[QNT_OCTAVE_OFFSET + 2*12] = (realtype)110.0;
    freqs[QNT_OCTAVE_OFFSET + 3*12] = (realtype)220.0;
    freqs[QNT_OCTAVE_OFFSET + 4*12] = (realtype)440.0;
    freqs[QNT_OCTAVE_OFFSET + 5*12] = (realtype)880.0;
    freqs[QNT_OCTAVE_OFFSET + 6*12] = (realtype)1760.0;
    freqs[QNT_OCTAVE_OFFSET + 7*12] = (realtype)3520.0;
    freqs[QNT_OCTAVE_OFFSET + 8*12] = (realtype)7040.0;
    freqs[QNT_OCTAVE_OFFSET + 9*12] = (realtype)14080.0;    
 
    /* Fill first six points from just above 20hz to 27.5 hz (4 octaves down from 440) */
    for (index=0; index < QNT_OCTAVE_OFFSET; index++)
    {                                                       
 	    freqs[index] = (realtype)(27.5 
 	    						/ pow(2.0, ((realtype)QNT_OCTAVE_OFFSET - (double)index)/12.0));
 	}
 	
	#define QNT_NUM_OCTAVES 9
 	/* Now loop thru octaves, all except last */
 	for( j=0; j<QNT_NUM_OCTAVES; j++)
 	{
    	int octave_index = QNT_OCTAVE_OFFSET + j*12;       
    	for (index=1; index < 12; index++)
    	{
    		int freq_index = octave_index + index;                                                       
    		
 	    	freqs[freq_index] = (realtype)( 
 	    		(double)freqs[octave_index] 
 	    		 * pow(2.0, ((double)index)/12.0));
 		}
 	}
 	
    /* Now fill last points */
    for (index=1; index < (MTH_NUM_PLOT_OCTAVE_PTS - (QNT_OCTAVE_OFFSET + QNT_NUM_OCTAVES*12)); index++)
    {                                                       
    	int freq_index = (QNT_OCTAVE_OFFSET + QNT_NUM_OCTAVES*12) + index;                                                       
    	
  	    freqs[freq_index] =  (realtype)( 
 	    		(double)freqs[(QNT_OCTAVE_OFFSET + QNT_NUM_OCTAVES*12)] 
 	    		 * pow(2.0, ((double)index)/12.0));
 	    		 
 	    /* Note that currently index 121 (122 points) is first point to contain 20480 */
 	    if( freqs[freq_index] > (realtype)20000.0 )
 	    	freqs[freq_index] = (realtype)20000.0;
 	}
 	
    /* Hard set end points */
    freqs[0] = (realtype)20.0;
    freqs[MTH_NUM_PLOT_OCTAVE_PTS - 1] = (realtype)20000.0;
    
    return(OKAY);
}

/*
 * FUNCTION: mthMidiOctaveFreqsPara()
 * DESCRIPTION:
 *
 *  Generates a special frequency range that covers 20 hz to 20000 hz
 *  with 128 points. The frequencies occur right on 440 tuning note values.
 */
int PT_DECLSPEC mthMidiOctaveFreqsPara( realtype *freqs, int num_pts ) 
{
    int j, index, shifted_index;
    realtype tmp_freqs[128];
/* #define NUM_INSERT_FREQS 22 For series that started at 55 */
#define NUM_INSERT_FREQS 6
    int flag60    = 1;
    int flag120   = 1;
    int flag180   = 1;
    int flag100   = 1;
    int flag200   = 1;
    int flag300   = 1;
    int flag400   = 1;
    int flag500   = 1;
    int flag600   = 1;
    int flag700   = 1;
    int flag800   = 1;
    int flag900   = 1;
    int flag1000  = 1;
    int flag2000  = 1;
    int flag3000  = 1;
    int flag4000  = 1;
    int flag5000  = 1;
    int flag6000  = 1;
    int flag7000  = 1;
    int flag8000  = 1;
    int flag9000  = 1;
    int flag10000 = 1;
    
  	if( freqs == NULL )
  		return(NOT_OKAY);
  	
    /* Hard set octave values. Choose 440 tuning octave locations to allow
     * good upper and lower endpoints. First point at 27.5 hz.
     */
    #define PARA_QNT_OCTAVE_OFFSET 6 
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 0*12] = (realtype)27.5;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 1*12] = (realtype)55.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 2*12] = (realtype)110.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 3*12] = (realtype)220.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 4*12] = (realtype)440.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 5*12] = (realtype)880.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 6*12] = (realtype)1760.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 7*12] = (realtype)3520.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 8*12] = (realtype)7040.0;
    tmp_freqs[PARA_QNT_OCTAVE_OFFSET + 9*12] = (realtype)14080.0;    
 
	/* Fill first points up to 27.5 */
	tmp_freqs[0] = (realtype)20.0;
	tmp_freqs[1] = (realtype)20.60172231;
	tmp_freqs[2] = (realtype)21.82676447;
	tmp_freqs[3] = (realtype)23.12465142;
	tmp_freqs[4] = (realtype)24.49971475;
	tmp_freqs[5] = (realtype)25.9565436;

	#define PARA_QNT_NUM_OCTAVES 9
 	/* Now loop thru octaves, all except last */
 	for( j=0; j<PARA_QNT_NUM_OCTAVES; j++)
 	{
    	int octave_index = PARA_QNT_OCTAVE_OFFSET + j*12;       
    	for (index=1; index < 12; index++)
    	{
    		int freq_index = octave_index + index;                                                       
    		
 	    	tmp_freqs[freq_index] = (realtype)( 
 	    		(double)tmp_freqs[octave_index] 
 	    		 * pow(2.0, ((double)index)/12.0));
 		}
 	}
 	
    /* Now fill last points */
    for (index=1; index < (128 - (PARA_QNT_OCTAVE_OFFSET + PARA_QNT_NUM_OCTAVES*12)); index++)
    {                                                       
    	int freq_index = (PARA_QNT_OCTAVE_OFFSET + PARA_QNT_NUM_OCTAVES*12) + index;                                                       
    	
  	    tmp_freqs[freq_index] =  (realtype)( 
 	    		(double)tmp_freqs[(PARA_QNT_OCTAVE_OFFSET + PARA_QNT_NUM_OCTAVES*12)] 
 	    		 * pow(2.0, ((double)index)/12.0));
 	    		 
 	    /* Note that currently index 121 (122 points) is first point to contain 20480 */
 	    if( tmp_freqs[freq_index] > (realtype)20000.0 )
 	    	tmp_freqs[freq_index] = (realtype)20000.0;
 	}
 	
    /* Now fill output array to requested point number. Insert special frequencies, shifting
     * the rest of the array up
     */
    shifted_index = 0;
    for(index=0; ((index<(num_pts - NUM_INSERT_FREQS)) && (shifted_index < 128)); index++)
    {
	    if( flag60 && (tmp_freqs[index] >= (realtype)60.0) )
	    {
    		freqs[shifted_index] = (realtype)60.0;	    	
    		flag60 = 0;
    		shifted_index++;
    	}
	    if( flag120 && (tmp_freqs[index] >= (realtype)120.0) )
	    {
    		freqs[shifted_index] = (realtype)120.0;	    	
    		flag120 = 0;
    		shifted_index++;
    	}
	    if( flag180 && (tmp_freqs[index] >= (realtype)180.0) )
	    {
    		freqs[shifted_index] = (realtype)180.0;	    	
    		flag180 = 0;
    		shifted_index++;
    	}
	    if( flag100 && (tmp_freqs[index] >= (realtype)100.0) )
	    {
    		freqs[shifted_index] = (realtype)100.0;	    	
    		flag100 = 0;
    		shifted_index++;
    	}
	    if( flag1000 && (tmp_freqs[index] >= (realtype)1000.0) )
	    {
    		freqs[shifted_index] = (realtype)1000.0;	    	
    		flag1000 = 0;
    		shifted_index++;
    	}
	    if( flag10000 && (tmp_freqs[index] >= (realtype)10000.0) )
	    {
    		freqs[shifted_index] = (realtype)10000.0;	    	
    		flag10000 = 0;
    		shifted_index++;
    	}
    	
#ifdef MTH_UNDEF /* Next set if for series that starts at 55 hz */
	    if( flag200 && (tmp_freqs[index] >= (realtype)200.0) )
	    {
    		freqs[shifted_index] = (realtype)200.0;	    	
    		flag200 = 0;
    		shifted_index++;
    	}
	    if( flag300 && (tmp_freqs[index] >= (realtype)300.0) )
	    {
    		freqs[shifted_index] = (realtype)300.0;	    	
    		flag300 = 0;
    		shifted_index++;
    	}
	    if( flag400 && (tmp_freqs[index] >= (realtype)400.0) )
	    {
    		freqs[shifted_index] = (realtype)400.0;	    	
    		flag400 = 0;
    		shifted_index++;
    	}
	    if( flag500 && (tmp_freqs[index] >= (realtype)500.0) )
	    {
    		freqs[shifted_index] = (realtype)500.0;	    	
    		flag500 = 0;
    		shifted_index++;
    	}
	    if( flag600 && (tmp_freqs[index] >= (realtype)600.0) )
	    {
    		freqs[shifted_index] = (realtype)600.0;	    	
    		flag600 = 0;
    		shifted_index++;
    	}
	    if( flag700 && (tmp_freqs[index] >= (realtype)700.0) )
	    {
    		freqs[shifted_index] = (realtype)700.0;	    	
    		flag700 = 0;
    		shifted_index++;
    	}
	    if( flag800 && (tmp_freqs[index] >= (realtype)800.0) )
	    {
    		freqs[shifted_index] = (realtype)800.0;	    	
    		flag800 = 0;
    		shifted_index++;
    	}
	    if( flag900 && (tmp_freqs[index] >= (realtype)900.0) )
	    {
    		freqs[shifted_index] = (realtype)900.0;	    	
    		flag900 = 0;
    		shifted_index++;
    	}
	    if( flag2000 && (tmp_freqs[index] >= (realtype)2000.0) )
	    {
    		freqs[shifted_index] = (realtype)2000.0;	    	
    		flag2000 = 0;
    		shifted_index++;
    	}
	    if( flag3000 && (tmp_freqs[index] >= (realtype)3000.0) )
	    {
    		freqs[shifted_index] = (realtype)3000.0;	    	
    		flag3000 = 0;
    		shifted_index++;
    	}
	    if( flag4000 && (tmp_freqs[index] >= (realtype)4000.0) )
	    {
    		freqs[shifted_index] = (realtype)4000.0;	    	
    		flag4000 = 0;
    		shifted_index++;
    	}
	    if( flag5000 && (tmp_freqs[index] >= (realtype)5000.0) )
	    {
    		freqs[shifted_index] = (realtype)5000.0;	    	
    		flag5000 = 0;
    		shifted_index++;
    	}
	    if( flag6000 && (tmp_freqs[index] >= (realtype)6000.0) )
	    {
    		freqs[shifted_index] = (realtype)6000.0;	    	
    		flag6000 = 0;
    		shifted_index++;
    	}
	    if( flag7000 && (tmp_freqs[index] >= (realtype)7000.0) )
	    {
    		freqs[shifted_index] = (realtype)7000.0;	    	
    		flag7000 = 0;
    		shifted_index++;
    	}
	    if( flag8000 && (tmp_freqs[index] >= (realtype)8000.0) )
	    {
    		freqs[shifted_index] = (realtype)8000.0;	    	
    		flag8000 = 0;
    		shifted_index++;
    	}
	    if( flag9000 && (tmp_freqs[index] >= (realtype)9000.0) )
	    {
    		freqs[shifted_index] = (realtype)9000.0;	    	
    		flag9000 = 0;
    		shifted_index++;
    	}
#endif // MTH_UNDEF    	
    	
    	freqs[shifted_index] = tmp_freqs[index];    	
    	shifted_index++;
    	
    }
    
    return(OKAY);
}
