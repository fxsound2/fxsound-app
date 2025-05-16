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
#include "mry.h"
#include "mth.h"
#include "qnt.h"
#include "u_qnt.h"

/*
 * FUNCTION: qntIToRInit()
 * DESCRIPTION:
 *  Allocates and initializes the passed qnt handle.
 * NOTES: 
 *	 i_output_quantized on will force the output values to be quantized
 *  to the number of levels specified by i_num_output_levels.  Note that
 *  this option is only useful when the number of requested output levels
 *  is less than the number of input levels.  If the i_output_quantized
 *  flag is off, then the number of output levels is equal to the number of
 *  input levels. i_output_quantized on also causes the values to be snapped (see below).
 *
 *  i_force_value_flag on will force the value in r_force_value to be a
 *  value in the output range, at the location closest to its value.
 *  If the specified output range is - the specified input range and the
 *  f_force_value is 0.0, then this forces an output range symmetric about 0.0.
 *
 *  i_snap_flag on cause the output values to be quantized to decimal values
 *  with as high a resolution possible given the specified input and output ranges
 *	 and the requested number of output levels.  The endpoints of the output
 *  range will always be the same as the specifed output range.  Note that turning
 *  this option on will typically lower the resolution of the knob. If the 
 *  i_output_quantized flag is on then snap is implied, then the output will snap
 *  in a decimal sense to the lesser number of output values.
 *
 *  The i_response_type flag sets the shape of the input to output response curve.
 *  When set to QNT_RESPONSE_LINEAR, the in to out response is linear.
 *
 *  When set to QNT_RESPONSE_MIDI_VOLUME_DISPLAY the response curve is a special version
 *  for use in displaying db level for volume control knobs and sliders.
 *	 Note that the output range specification is in terms of db.  The r_output_max
 *  value is interpreted as the linear gain number that is converted to a db
 *  level in generating the output values. 
 *	
 *	 When the response flag is set to QNT_RESPONSE_VOLUME_DSP, the response curve
 *	 is the values needed to be sent to the DSP to implement the db based response
 *  described above. It can also be used to generate the DISPLAY values on a linear scale.
 *  IMPORTANT - care must be taken to call the inits for the DSP
 *  and DISPLAY handles with exactly the same input parameters to insure the DISPLAY
 *  and DSP values are consistent.
 *
 *	 When set to QNT_RESPONSE_EXP, if the i_snap_flag is set, then the values are
 *  quantized to "nice" decimal values, with the quanization controlled by the
 *  number of decimal places set by i_num_output_levels, which sets the decimal place
 *  from the most significant that controls the rounding for each value.
 */
int PT_DECLSPEC qntIToRInit(PT_HANDLE **hpp_qnt, CSlout *hp_slout,
					 int i_input_min, int i_input_max,
					 realtype r_output_min, realtype r_output_max,
					 int i_output_quantized, int i_num_output_levels,
					 int i_force_value_flag, realtype r_force_value,
					 int i_snap_flag, int i_response_type)
{
  struct qntHdlType *cast_handle;
  int index;
  int array_size;
  int num_actual_outputs;
 
  /* Make sure output level request is legal */
  array_size = i_input_max - i_input_min + 1;
  if (i_output_quantized)
  {
     if (i_num_output_levels > array_size)
        return(NOT_OKAY);
  }
    
  /* Allocate the handle */
  cast_handle = (struct qntHdlType *)calloc(1,
						sizeof(struct qntHdlType));
  if (cast_handle == NULL)
	return(NOT_OKAY);

  cast_handle->slout_hdl = hp_slout;
  cast_handle->in_out_mode = QNT_INT_TO_REAL;
  cast_handle->min_int_input = i_input_min;
  cast_handle->int_array = NULL;
  cast_handle->long_array = NULL;
  cast_handle->i_force_value_index = 0; /* Init index to legal value */
    
  /* Allocate the array of reals */
  cast_handle->array_size = array_size;
  cast_handle->real_array = (realtype *)calloc(cast_handle->array_size,
				               sizeof(realtype));
  if (cast_handle->real_array == NULL)
	return(NOT_OKAY);   
		
  if ( (i_response_type == QNT_RESPONSE_LINEAR ) || (i_response_type == QNT_RESPONSE_LINEAR_NO_ROUND) )
  {	
    /* Set value for actual number of output values */
    if( i_output_quantized )
    	num_actual_outputs = i_num_output_levels;
    else
    {   
    	num_actual_outputs = array_size;
    	if( i_force_value_flag )
    		num_actual_outputs -= 1; /* Make room for repeated force value point */    	
    }
	
	/* First handle the case of a forced value- requires different indexing */    
    if( i_force_value_flag )
    {
    	int force_value_index = 0;
    	int warped_index;
    	realtype difference = (realtype)1.0E37;
    	realtype trial_difference;
    	/* Note scale has to be compensated for repeated point (-1 term) */
        realtype scale =  (r_output_max - r_output_min)/(i_input_max - i_input_min - 1);
   	
        /* First find index of point closest to force value point */
        for (index=0; index < (array_size -1); index++)
        {
        	trial_difference = (realtype)fabs( (r_output_min + scale * index) - r_force_value );
        	if( trial_difference < difference )
        	{
        		force_value_index = index;
        		difference = trial_difference;
        	}
        }
        cast_handle->i_force_value_index = force_value_index; /* For use in initializing knob */
        
        /* Now fill out array values, with a repeated force_value location */
        warped_index = 0;
        for (index=0; index < (array_size); index++)
        {                                
        	if( index > force_value_index )
        		warped_index = index - 1;
        	else
        		warped_index = index;
        	if( (index == force_value_index) || (index == (force_value_index + 1) ))
        		cast_handle->real_array[index] = r_force_value;
        	else
				cast_handle->real_array[index] = r_output_min + scale * warped_index;
        }                            
    }   
    /* For no forced value case, fill out array of unsnapped values */
    else
    {	
    	/* Note input range spans full 127 steps for this scale */
    	realtype scale = (r_output_max - r_output_min)/(i_input_max - i_input_min);
    	
    	for (index=0; index < (array_size); index++)
    	{
 			cast_handle->real_array[index] = r_output_min + scale * index;
 		} 
    }

    /* Hard set endpoint to override any roundoff inaccuracies */
    cast_handle->real_array[(cast_handle->array_size - 1)] = r_output_max;
    
    /* Now handle output quantized and snap value case with rounding. This puts
	  * on the special decimal oriented rounding for display with limited digits.
	  */
    if( ((i_output_quantized) || (i_snap_flag)) && !(i_response_type == QNT_RESPONSE_LINEAR_NO_ROUND) )    	
    {   
       realtype delta_val;
    
       mthCalcQuantDelta(r_output_min, r_output_max, num_actual_outputs, &delta_val);
       /* Set variable for warping functions */
       cast_handle->half_delta = delta_val * (realtype)0.5;
    
       /* Fill all points except first point and final point */
       for (index=0; index < (cast_handle->array_size); index++)
          cast_handle->real_array[index] = mthCalcRoundedValue(cast_handle->real_array[index],
          													 delta_val, r_output_min, r_output_max);
    }       
    /* Now handle output quantized no rounding requested */
    if( (i_output_quantized)  && (i_response_type == QNT_RESPONSE_LINEAR_NO_ROUND) )    	
    {   
       realtype delta_val = (r_output_max - r_output_min)/(realtype)(num_actual_outputs - 1);
    
       /* Round all the filled points to the delta values */
       for (index=0; index < (cast_handle->array_size); index++)
          cast_handle->real_array[index] -= (realtype)fmod(cast_handle->real_array[index], delta_val);
    }       
  /* End of QNT_RESPONSE_LINEAR case */
  }
  
  /* Now handle volume control cases */
  if ( (i_response_type == QNT_RESPONSE_MIDI_VOLUME_DISPLAY) 
                                || (i_response_type == QNT_RESPONSE_MIDI_VOLUME ))
  { 
  	/* Pure db based settings (THIS WAS TOO WARPED)
    for (index=1; index < (cast_handle->array_size - 1); index++)
 	    cast_handle->real_array[index] = (realtype)pow(10.0, 0.05 * cast_handle->real_array[index]);
 	 */
 	 
    /* A squared response is currently being used */
    realtype scale = (realtype)1.0/((realtype)i_input_max - (realtype)i_input_min);
 
    for (index=1; index < (cast_handle->array_size - 1); index++)
    {                                                       
    	realtype tmp = index * scale;
 	    cast_handle->real_array[index] = tmp * tmp * r_output_max;
	 }
 	 
    /* Hard set end points */
    cast_handle->real_array[0] = 0.0;
    cast_handle->real_array[(cast_handle->array_size - 1)] = r_output_max;
  }
  /* Do additional operations for QNT_RESPONSE_MIDI_VOLUME_DISPLAY case */
  if ( i_response_type == QNT_RESPONSE_MIDI_VOLUME_DISPLAY )
  {
     /* Now set up db Display Values */
  	  realtype r_output_max_db = (realtype)20.0 * (realtype)log10( r_output_max );
  	  /* Next value determines the db level of the first non-zero tick on the knob */
  	  realtype r_output_min_db = (realtype)QNT_RESPONSE_MIDI_VOLUME_DB_MIN;
  	
     for (index=1; index < (array_size - 1); index++)
 	    cast_handle->real_array[index] 
 		           = (realtype)20.0 * (realtype)log10( cast_handle->real_array[index] );

     /* Hard set endpoints to override any roundoff inaccuracies */
     cast_handle->real_array[0] = QNT_DB_VOLUME_OFF;
     cast_handle->real_array[(cast_handle->array_size - 1)] = r_output_max_db;
    /* End of QNT_RESPONSE_VOLUME_DISPLAY case */
  }
  
  /* Now handle case of special two part linear control curve.
   * This uses two linear sections, with the first section having lower
   * slope to give more resolution on the lower end. The location of the
   * breakpoint depends on the ratio of the start point to the end point.
   * The function assumes values are to be snapped to ..., .1, .2, .5, series,
   * irregardless of setting of i_snapflag.
   * Extra points left over are then used to create first linear section.
   */
  if( i_response_type == QNT_RESPONSE_TWO_PART_LINEAR )
  {
     realtype delta_val;
     int num_of_1rst_seg_pts, num_of_2nd_seg_pts;
    
     num_actual_outputs = array_size;
     
	  /* First find delta snap value for these input end points */
     mthCalcQuantDelta(r_output_min, r_output_max, num_actual_outputs, &delta_val);
     /* Now find the number of points needed to span snapped range */
     num_of_2nd_seg_pts = (int)( r_output_max/delta_val + (realtype)0.5 );
     /* Now use extra points in first segment */
     num_of_1rst_seg_pts = num_actual_outputs - num_of_2nd_seg_pts;
     
     /* Set variable for warping functions */
     cast_handle->half_delta = delta_val * (realtype)0.5;
    
	 /* First fill lower segment of two part curve */
	 {	     
    	/* Note input range spans full 127 steps for this scale */
    	realtype scale = (delta_val - r_output_min)/(num_of_1rst_seg_pts);
    	
    	for (index=0; index < (num_of_1rst_seg_pts); index++)
    	{
 			cast_handle->real_array[index] = r_output_min + scale * index;            
 			/* Currently, don't snap lower segment points.
        	cast_handle->real_array[index] = mthCalcRoundedValue(cast_handle->real_array[index],
          													 delta_val, r_output_min, r_output_max);
          	*/
 		}
	 }
	 
	 /* Now fill upper segment of two segment curve */
	 {	     
    	for (index=1; index < num_of_2nd_seg_pts; index++)
    	{                                   
    		int index2 = index + num_of_1rst_seg_pts - 1;
 			cast_handle->real_array[index2] = index * delta_val;
 		} 

    	/* Hard set endpoint to override any roundoff inaccuracies */
    	cast_handle->real_array[(cast_handle->array_size - 1)] = r_output_max;
	 }
  }
  
  /* Now handle exponential control case */
  if ( (i_response_type == QNT_RESPONSE_EXP) ) 
  {
	  realtype factor;
  	
  	  if( r_output_min <= (realtype)0.0 )
  		 return(NOT_OKAY);
  	
  	  factor = (realtype)pow((double)(r_output_max/r_output_min), (double)(1.0/i_input_max));
  	
     realtype scale = (realtype)1.0/((realtype)i_input_max - (realtype)i_input_min);
    
     /* Hard set end points */
     cast_handle->real_array[0] = r_output_min;
     cast_handle->real_array[(cast_handle->array_size - 1)] = r_output_max;
 
     for (index=1; index < (cast_handle->array_size - 1); index++)
     {                                                       
 	     cast_handle->real_array[index] = cast_handle->real_array[index - 1] * factor;
	  }

	  if (i_force_value_flag)
	  {
		  /* Do additional operations for this case, converting value into closest
			* quantized fractional decimal value. For example, if r_force_value is 2.5,
			* value will be quantized to the series 90,100,105,110,...
			*/
		  for (index=1; index < (cast_handle->array_size - 1); index++)
		  {                                                       
 			  cast_handle->real_array[index] = 
				  mthCalcClosestNiceValue(cast_handle->real_array[index], r_force_value);
		  } 	 
	  }
  }
  
  /* Now handle exponential factor control case. This is for knobs such as a frequency
   * fine control that moves the main frequency up and down an octave.
   * The center value is r_force_value.
   */
  if ( i_response_type == QNT_RESPONSE_EXP_FACTOR ) 
  {
  	  realtype factor_up   = (realtype)pow((double)(r_output_max/r_force_value), (double)(1.0/63.0));
  	  realtype factor_down = (realtype)pow((double)(r_output_min/r_force_value), (double)(1.0/63.0));
  	
  	  cast_handle->real_array[63] = r_force_value;
  	  cast_handle->real_array[64] = r_force_value;
  	
  	  for(index=1; index<64; index++)
	  {
  		 cast_handle->real_array[64 + index] = (realtype)pow((double)factor_up, (double)index);
  		 cast_handle->real_array[63 - index] = (realtype)pow((double)factor_down, (double)index);
	  }
  		
     /* Hard set end points */
     cast_handle->real_array[0] = r_output_min;
     cast_handle->real_array[127] = r_output_max;
  }
  
  /* Now handle special exponential audio frequency control case.
   * This puts frequencies right on note values given 440 tuning.
   * Endpoints are added at 20hz and 20khz.
   */
  if ( i_response_type == QNT_RESPONSE_EXP_FREQ ) 
  {    
   	if( mthMidiOctaveFreqsPara( cast_handle->real_array, array_size ) != OKAY )
   		return(NOT_OKAY);  	
  }
   
  /* Now handle special case for Q filter knobs.
   * Hard coded for MIDI case (127 steps)
   */
  if ( i_response_type == QNT_RESPONSE_Q_TYPE ) 
  {
  	  realtype *q = cast_handle->real_array;
  	  realtype val;
  	  int i;

	  /* Note - take care when changing these ranges, since if a prior
	   * range fills all 128 points, next range will access bad memory.
	   */
	  index = 127;
	  i = 0;
	  /* Fill points from 0.2 to 1.0 */
	  do
	  {
		  val = q[index] = (realtype)( 0.2 + 0.025 * (double)i);
		  index--;
		  i++;
	  }
	  while( (val < (realtype)1.0) && (index >= 0) );
	
	  /* Fill points from 1.0 to 1.5 */
	  i = 0;
	  do
	  {
		  val = q[index] = (realtype)( 1.0 + 0.05 * (double)i);
		  index--;
		  i++;
	  }
	  while( (val < (realtype)1.5) && (index >= 0) );
		
	  /* Fill points from 1.5 to 5.0 */
	  i = 0;
	  do
	  {
		  val = q[index] = (realtype)( 1.5 + 0.1 * (double)i);
		  index--;
		  i++;
	  }
	  while( (val < (realtype)5.0) && (index >= 0) );
		
	  /* Fill points from 5.0 to 10.0 */
	  i = 0;
	  do
	  {
		  val = q[index] = (realtype)( 5.0 + 0.2 * (double)i);
		  index--;
		  i++;
	  }
	  while( (val < (realtype)10.0) && (index >= 0) );
	
	  /* Fill points from 10.0 to 20.0 */
	  i = 0;
	  do
	  {
		  val = q[index] = (realtype)( 10.0 + 0.5 * (double)i);
		  index--;
		  i++;
	  }
	  while( (val < (realtype)20.0) && (index >= 0) );
	
	  q[0] = (realtype)20.0;	
  }

  /* Now handle square root control case */
  if ( (i_response_type == QNT_RESPONSE_SQRT) ) 
  {
  	  if( (r_output_min < (realtype)0.0) || (r_output_max < (realtype)0.0) )
  		 return(NOT_OKAY);

     /* First hard set end points */
     cast_handle->real_array[0] = r_output_min;
     cast_handle->real_array[(cast_handle->array_size - 1)] = r_output_max;

	  /* To get correct intermediate values, first square endpoints */
	  r_output_min *= r_output_min;
	  r_output_max *= r_output_max;
  	
     realtype scale = (realtype)(r_output_max - r_output_min)/((realtype)i_input_max - (realtype)i_input_min);
    
     for (index=1; index < (cast_handle->array_size - 1); index++)
     {                                                       
 	     cast_handle->real_array[index] = (realtype)sqrt(r_output_min + index * scale);
	  }
  }

  /* Now handle special case for maximizer boost */
  if ( (i_response_type == QNT_RESPONSE_MAXI_BOOST) || (i_response_type == QNT_RESPONSE_MAXI_MAX_OUTPUT)
	    || (i_response_type == QNT_RESPONSE_MAXI_BOOST_DSP) || (i_response_type == QNT_RESPONSE_MAXI_MAX_OUTPUT_DSP) )
  {
  	  realtype scale, tmp_val;
	  int v_index;
    
     /* Hard set end points */
     cast_handle->real_array[0] = (realtype)0.0;
     cast_handle->real_array[(cast_handle->array_size - 1)] = (realtype)30.0;
 
	  index = 0;
	  v_index = 0;
	  scale = (realtype)0.1;
	  while ( (tmp_val = v_index * scale) < (realtype)6.0 )
	  {
  		  cast_handle->real_array[index] = tmp_val;
		  index++;
		  v_index++;
	  }

     v_index = 0;
	  scale = (realtype)0.2;
	  while ( (tmp_val = v_index * scale + (realtype)6.0) < (realtype)12.0 )
	  {
  		  cast_handle->real_array[index] = tmp_val;
		  index++;
		  v_index++;
	  }

     v_index = 0;
	  scale = (realtype)0.5;
	  while ( (tmp_val = v_index * scale + (realtype)12.0) < (realtype)30.0 )
	  {
  		  cast_handle->real_array[index] = tmp_val;
		  index++;
		  v_index++;
	  }
     /* With fills above, one extra point, set it */
	  cast_handle->real_array[126] = cast_handle->real_array[127];

     if( (i_response_type == QNT_RESPONSE_MAXI_MAX_OUTPUT) || (i_response_type == QNT_RESPONSE_MAXI_MAX_OUTPUT_DSP) )
	  {
		  float tmp_val;

		  /* Reverse and negate values in this case */
		  for(index=0; index < cast_handle->array_size/2; index++)
		  {
				tmp_val = - cast_handle->real_array[index];
				cast_handle->real_array[index] = - cast_handle->real_array[cast_handle->array_size - index - 1];
				cast_handle->real_array[cast_handle->array_size - index - 1] = tmp_val;
		  }
	  }

     if( (i_response_type == QNT_RESPONSE_MAXI_BOOST_DSP) || (i_response_type == QNT_RESPONSE_MAXI_MAX_OUTPUT_DSP) )
	  {
		  /* For DSP, convert db figures to linear values */
		  for(index=0; index < cast_handle->array_size; index++)
				cast_handle->real_array[index] =  
				   (realtype)pow((double)10.0, (double)cast_handle->real_array[index]/(realtype)20.0);
	  }
  }
  *hpp_qnt = (PT_HANDLE *)cast_handle;

  return(OKAY);
}
