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
 * FUNCTION: mthConvert16BitIntBufToRealtype()
 * DESCRIPTION:
 *  For converting a 16 bit short int buffer to a realtype buffer..
 */
int PT_DECLSPEC mthConvert16BitIntBufToRealtype(int i_length, short int *sp_buf, float *rp_buf)
{	
	int i;
	for(i=0; i<i_length; i++)
	{
		rp_buf[i] = (float)(sp_buf[i]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);
	}

	return(OKAY);
}

/*
 * FUNCTION: mthConvertIntBufToRealtype()
 * DESCRIPTION:
 *  For converting an int buffer of the specified bit width to a realtype buffer.
 */
int PT_DECLSPEC mthConvertIntBufToRealtype(int i_length, int i_bit_width, int i_bits_valid, short int *sp_buf, float *rp_buf)
{	
	int i;
	unsigned char *unsigned_char_p;
	int *int_p;

	switch( i_bit_width )
	{
	case 16:
		/* Most common case first */
	   for(i=0; i<i_length; i++)
		{
		   rp_buf[i] = (float)(sp_buf[i]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	case 8:
		/* Second most common case.
		 * 8 bit .wav data is internally represented
		 * as an unsigned char value from 0 to 255 with 128 as zero signal.
		 */
		unsigned_char_p = (unsigned char *)sp_buf;
		int i_tmp;

	   for(i=0; i<i_length; i++)
		{
			i_tmp = unsigned_char_p[i] - 128;
		   rp_buf[i] = (float)i_tmp * (float)(1.0/MTH_8_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	// The 24 bit case will sometimes use a "bits valid" flag that specifies the location of the MSB
	case 24:
		/* Note that with Windows wave based calls, internally the 24 bit data is packed as 3 consecutive bytes, with no "empty" byte. */
		unsigned_char_p = (unsigned char *)sp_buf;
		short int val_16_bit;
		int val_24_bit;

		switch( i_bits_valid )
		{
		case 16:
			for(i=0; i<i_length; i++)
			{
				// Build an int value out of the 3 bytes that form the 20 bit value.
				val_16_bit = (char)unsigned_char_p[2];
				val_16_bit = (val_16_bit << 8) | unsigned_char_p[1];

				rp_buf[i] = (float)(val_16_bit) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);
				
				unsigned_char_p += 3;
			}
			break;

		case 20:
			for(i=0; i<i_length; i++)
			{
				// Build and int value out of the 3 bytes that form the 20 bit value.
				val_24_bit = (char)unsigned_char_p[2];
				val_24_bit = (val_24_bit << 8) | unsigned_char_p[1];
				val_24_bit = (val_24_bit << 4) | (unsigned_char_p[0] >> 4);

				rp_buf[i] = (float)(val_24_bit) * (float)(1.0/MTH_20_BIT_REAL_CONVERSION_FACTOR);
				
				unsigned_char_p += 3;
			}
			break;

		case 24:
			for(i=0; i<i_length; i++)
			{
				// Build and int value out of the 3 bytes that form the 24 bit value.
				val_24_bit = (char)unsigned_char_p[2];
				val_24_bit = (val_24_bit << 8) | unsigned_char_p[1];
				val_24_bit = (val_24_bit << 8) | (unsigned_char_p[0]);

				rp_buf[i] = (float)(val_24_bit) * (float)(1.0/MTH_24_BIT_REAL_CONVERSION_FACTOR);
				
				unsigned_char_p += 3;
			}
			break;

		default:
		/* Not a supported valid bit setting */
			return(NOT_OKAY);
		}
	break;

	case 32:
	{
		unsigned_char_p = (unsigned char *)sp_buf;
		short int val_16_bit;
		int val_24_bit;

		switch( i_bits_valid )
		{
		case 16:
			for(i=0; i<i_length; i++)
			{
				// Build and int value out of the 2 bytes that form the 16 bit value.
				val_16_bit = (char)unsigned_char_p[3];
				val_16_bit = (val_16_bit << 8) | unsigned_char_p[2];

				rp_buf[i] = (float)(val_16_bit) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);
				
				unsigned_char_p += 4;
			}
			break;

		case 20:
			for(i=0; i<i_length; i++)
			{
				// Build and int value out of the 3 bytes that form the 20 bit value.
				val_24_bit = (char)unsigned_char_p[3];
				val_24_bit = (val_24_bit << 8) | unsigned_char_p[2];
				val_24_bit = (val_24_bit << 4) | (unsigned_char_p[1] >> 4);

				rp_buf[i] = (float)(val_24_bit) * (float)(1.0/MTH_20_BIT_REAL_CONVERSION_FACTOR);
				
				unsigned_char_p += 4;
			}
			break;

		case 24:
			for(i=0; i<i_length; i++)
			{
				// Build and int value out of the 3 bytes that form the 20 bit value.
				val_24_bit = (char)unsigned_char_p[3];
				val_24_bit = (val_24_bit << 8) | unsigned_char_p[2];
				val_24_bit = (val_24_bit << 8) | (unsigned_char_p[1]);

				rp_buf[i] = (float)(val_24_bit) * (float)(1.0/MTH_24_BIT_REAL_CONVERSION_FACTOR);
				
				unsigned_char_p += 4;
			}
			break;

		case 32:
			int_p = (int *)sp_buf;
			for(i=0; i<i_length; i++)
			{
				rp_buf[i] = (float)(int_p[i]) * (float)(1.0/MTH_32_BIT_REAL_CONVERSION_FACTOR);
			}
			break;

		default:
		/* Not a supported valid bit setting */
			return(NOT_OKAY);
		}
	}
	break;

	default:
		/* Not a supported bit width */
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: mthConvertIntBufToRealtypeSurroundPreProcess()
 * DESCRIPTION:
 *  For converting an int buffer of the specified bit width to a realtype buffer.
 *  This special version also reorders the buffer for SurroundSound processing by DFX processors.
 *  Front channel array as ordered in pairs is first in output, followed by Rear channel array in ordered pairs
 *  and then array of center mono channel signal points and then array of subwoofer mono channel signal points.
 *  Note that i_length is the total length of the input and output arrays.
 */
int PT_DECLSPEC mthConvertIntBufToRealtypeSurroundPreProcess(int i_length, int i_bit_width, int i_bits_valid, short int *sp_buf, float *rp_buf)
{	
	int i, j, k;
	unsigned char *ucp;
	int *int_p;
	int array_length;

	array_length = i_length/6;

	switch( i_bit_width )
	{
	case 16:
		/* Most common case first */
	   for(i=0; i<array_length; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
		   rp_buf[j] = (float)(sp_buf[k]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);
		   rp_buf[j + 1] = (float)(sp_buf[k+1]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);

			// Rear channels
		   rp_buf[array_length * 2 + j] = (float)(sp_buf[k+2]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);
		   rp_buf[array_length * 2 + j + 1] = (float)(sp_buf[k+3]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);

			// Center channel
		   rp_buf[array_length * 4 + i] = (float)(sp_buf[k+4]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);

			// Subwoofer channel
		   rp_buf[array_length * 5 + i] = (float)(sp_buf[k+5]) * (float)(1.0/MTH_16_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	case 8:
		/* Second most common case.
		 * 8 bit .wav data is internally represented
		 * as an unsigned char value from 0 to 255 with 128 as zero signal.
		 */
		ucp = (unsigned char *)sp_buf;

	   for(i=0; i<array_length; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
		   rp_buf[j] = (float)(ucp[k] - 128) * (float)(1.0/MTH_8_BIT_REAL_CONVERSION_FACTOR);
		   rp_buf[j + 1] = (float)(ucp[k+1] - 128) * (float)(1.0/MTH_8_BIT_REAL_CONVERSION_FACTOR);

			// Rear channels
		   rp_buf[array_length * 2 + j] = (float)(ucp[k+2] - 128) * (float)(1.0/MTH_8_BIT_REAL_CONVERSION_FACTOR);
		   rp_buf[array_length * 2 + j + 1] = (float)(ucp[k+3] - 128) * (float)(1.0/MTH_8_BIT_REAL_CONVERSION_FACTOR);

			// Center channel
		   rp_buf[array_length * 4 + i] = (float)(ucp[k+4] - 128) * (float)(1.0/MTH_8_BIT_REAL_CONVERSION_FACTOR);

			// Subwoofer channel
		   rp_buf[array_length * 5 + i] = (float)(ucp[k+5] - 128) * (float)(1.0/MTH_8_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	// Currently not supporting 24 bit format
	case 24:
		return(NOT_OKAY);
	break;

	case 32:
		/* Least common case last */
		int_p = (int *)sp_buf;
	   for(i=0; i<array_length; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
		   rp_buf[j] = (float)(int_p[k]) * (float)(1.0/MTH_32_BIT_REAL_CONVERSION_FACTOR);
		   rp_buf[j + 1] = (float)(int_p[k+1]) * (float)(1.0/MTH_32_BIT_REAL_CONVERSION_FACTOR);

			// Rear channels
		   rp_buf[array_length * 2 + j] = (float)(int_p[k+2]) * (float)(1.0/MTH_32_BIT_REAL_CONVERSION_FACTOR);
		   rp_buf[array_length * 2 + j + 1] = (float)(int_p[k+3]) * (float)(1.0/MTH_32_BIT_REAL_CONVERSION_FACTOR);

			// Center channel
		   rp_buf[array_length * 4 + i] = (float)(int_p[k+4]) * (float)(1.0/MTH_32_BIT_REAL_CONVERSION_FACTOR);

			// Subwoofer channel
		   rp_buf[array_length * 5 + i] = (float)(int_p[k+5]) * (float)(1.0/MTH_32_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	default:
		/* Not a supported bit width */
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: mthConvertRealtypeBufTo16BitIntBuf()
 * DESCRIPTION:
 *  For converting a realtype buffer to a short int buffer.
 *  Note that this version assumes the real values have already
 *  been limited to +/- 1 and does not clip the real before conversion.
 */
int PT_DECLSPEC mthConvertRealtypeBufTo16BitIntBuf(int i_length, float *rp_buf, short int *sp_buf)
{	
	int i;

	for(i=0; i<i_length; i++)
	{
		sp_buf[i] = (short int)(rp_buf[i] * (float)32768.0);
	}

	return(OKAY);
}

/*
 * FUNCTION: mthConvertRealtypeBufTo16BitIntBufWithClipping()
 * DESCRIPTION:
 *  For converting a realtype buffer to a short int buffer.
 *  Note that this version assumes the real values have already
 *  been limited to +/- 1 and does not clip the real before conversion.
 */
int PT_DECLSPEC mthConvertRealtypeBufTo16BitIntBufWithClipping(int i_length, realtype *rp_buf, short int *sp_buf)
{	
	int i;

	for(i=0; i<i_length; i++)
	{
		realtype r_tmp;

		r_tmp = rp_buf[i] * (realtype)32768.0;

		if( r_tmp < (realtype)-32768.0 )
			sp_buf[i] = (short int)-32768;
		else
		{
			if( r_tmp > (realtype) 32767.0 )
				sp_buf[i] = (short int)32767;
			else
				sp_buf[i] = (short int)r_tmp;
		}
	}

	return(OKAY);
}

/*
 * FUNCTION: mthConvertRealtypeBufToIntBuf()
 * DESCRIPTION:
 *  For converting a realtype buffer to a specified precision integer buffer.
 *  Note that this version assumes the real values have already
 *  been limited to +/- 1 and does not clip the real before conversion.
 *
 * 
 */
int PT_DECLSPEC mthConvertRealtypeBufToIntBuf(int i_length, int i_bit_width, int i_bits_valid, float *rp_buf, short int *sp_buf)
{	
	int i;
	unsigned char *unsigned_char_p;
	unsigned char *unsigned_char_p2;
	int *int_p;
	int i_tmp;

	// In 8 and 16 bit case all bits are always valid
	switch( i_bit_width )
	{
	case 16:
		for(i=0; i<i_length; i++)
		{
			sp_buf[i] = (short int)(rp_buf[i] * (float)MTH_16_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	case 8:
		/* 8 bit data is internally represented
		 * as an unsigned char value from 0 to 255 with 128 as zero signal.
		 */
		unsigned_char_p = (unsigned char *)sp_buf;
	   for(i=0; i<i_length; i++)
		{
			i_tmp = (int)(rp_buf[i] * (float)MTH_8_BIT_REAL_CONVERSION_FACTOR);
			unsigned_char_p[i] = i_tmp + 128;
		}
		break;

	// The 24 bit case uses a "bits valid" setting that specifies the location of the MSB
	case 24:
		unsigned_char_p = (unsigned char *)sp_buf;
		unsigned_char_p2 = (unsigned char *)&i_tmp;
		switch( i_bits_valid )
		{

		case 16:
			for(i=0; i<i_length; i++)
			{
				i_tmp = (int)(rp_buf[i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
				unsigned_char_p[1] = unsigned_char_p2[2];
				unsigned_char_p[2] = unsigned_char_p2[3];
				unsigned_char_p += 3;
			}
			break;

		case 20:
			for(i=0; i<i_length; i++)
			{
				i_tmp = (int)(rp_buf[i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
				i_tmp = i_tmp >> 4;
				unsigned_char_p[0] = unsigned_char_p2[1];
				unsigned_char_p[1] = unsigned_char_p2[2];
				unsigned_char_p[2] = unsigned_char_p2[3];
				unsigned_char_p += 3;
			}
			break;

		case 24:
			for(i=0; i<i_length; i++)
			{
				i_tmp = (int)(rp_buf[i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
				unsigned_char_p[0] = unsigned_char_p2[1];
				unsigned_char_p[1] = unsigned_char_p2[2];
				unsigned_char_p[2] = unsigned_char_p2[3];
				unsigned_char_p += 3;
			}
			break;
		}
		break;

	// The 32 bit case will use a "bits valid" flag that specifies the location of the MSB
	case 32:
		int_p = (int *)sp_buf;

		switch( i_bits_valid )
		{
		case 16:
			for(i=0; i<i_length; i++)
			{
				i_tmp = (int)(rp_buf[i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
				int_p[i] = (i_tmp >> 16);
			}
			break;

		case 20:
			for(i=0; i<i_length; i++)
			{
				i_tmp = (int)(rp_buf[i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
				int_p[i] = (i_tmp >> 12);
			}
			break;

		case 24:
			for(i=0; i<i_length; i++)
			{
				i_tmp = (int)(rp_buf[i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
				int_p[i] = (i_tmp >> 8);
			}
			break;

		case 32:
			for(i=0; i<i_length; i++)
			{
				int_p[i] = (int)(rp_buf[i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
			}
			break;
		}

	default:
		/* Not a supported bit width */
		return(NOT_OKAY);
	}

	return(OKAY);
}

/*
 * FUNCTION: mthConvertRealtypeBufToIntBufSurroundPostProcess()
 * DESCRIPTION:
 *  For converting a realtype buffer to a specified precision integer buffer.
 *  Note that this version assumes the real values have already
 *  been limited to +/- 1 and does not clip the real before conversion.
 *  This special version also reorders the buffer for SurroundSound processing by DFX processors.
 *  Assumes in incoming buffer that Front channel array as ordered in pairs is first, followed by Rear channel array in ordered pairs
 *  and then array of center mono channel signal points and then array of subwoofer mono channel signal points.
 *  Channels are reordered to .wav file format as six point sets.
 *  Note that i_length is the total length of the input and output arrays.
 */
int PT_DECLSPEC mthConvertRealtypeBufToIntBufSurroundPostProcess(int i_length, int i_bit_width, int i_bits_valid, float *rp_buf, short int *sp_buf)
{	
	int i, j, k;
	unsigned char *ucp;
	int *int_p;
	int array_length;

	array_length = i_length/6;

	switch( i_bit_width )
	{
	case 16:
		/* Most common case first */
		for(i=0; i<array_length; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
			sp_buf[k]   = (short int)(rp_buf[j] * (float)MTH_16_BIT_REAL_CONVERSION_FACTOR);
			sp_buf[k+1] = (short int)(rp_buf[j+1] * (float)MTH_16_BIT_REAL_CONVERSION_FACTOR);

			// Rear channels
			sp_buf[k+2] = (short int)(rp_buf[array_length * 2 + j] * (float)MTH_16_BIT_REAL_CONVERSION_FACTOR);
			sp_buf[k+3] = (short int)(rp_buf[array_length * 2 + j + 1] * (float)MTH_16_BIT_REAL_CONVERSION_FACTOR);

			// Center channel
			sp_buf[k+4] = (short int)(rp_buf[array_length * 4 + i] * (float)MTH_16_BIT_REAL_CONVERSION_FACTOR);

			// Subwoofer channel
			sp_buf[k+5] = (short int)(rp_buf[array_length * 5 + i] * (float)MTH_16_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	case 8:
		/* Second most common case.
		 * 8 bit data is internally represented
		 * as an unsigned char value from 0 to 255 with 128 as zero signal.
		 */
		ucp = (unsigned char *)sp_buf;

	   for(i=0; i<array_length; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
			ucp[k]   = (int)(rp_buf[j] * (float)MTH_8_BIT_REAL_CONVERSION_FACTOR) + 128;
			ucp[k+1] = (int)(rp_buf[j+1] * (float)MTH_8_BIT_REAL_CONVERSION_FACTOR) + 128;

			// Rear channels
			ucp[k+2] = (int)(rp_buf[array_length * 2 + j] * (float)MTH_8_BIT_REAL_CONVERSION_FACTOR) + 128;
			ucp[k+3] = (int)(rp_buf[array_length * 2 + j + 1] * (float)MTH_8_BIT_REAL_CONVERSION_FACTOR) + 128;

			// Center channel
			ucp[k+4] = (int)(rp_buf[array_length * 4 + i] * (float)MTH_8_BIT_REAL_CONVERSION_FACTOR) + 128;

			// Subwoofer channel
			ucp[k+5] = (int)(rp_buf[array_length * 5 + i] * (float)MTH_8_BIT_REAL_CONVERSION_FACTOR) + 128;
		}
		break;

	// 24 bit currently not supported
	case 24:
		return(NOT_OKAY);
		break;

	case 32:
		/* Least common case last */
		int_p = (int *)sp_buf;
		for(i=0; i<array_length; i++)
		{
			j = i * 2;
			k = i * 6;

			// Front channels
			int_p[k]   = (int)(rp_buf[j] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
			int_p[k+1] = (int)(rp_buf[j+1] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);

			// Rear channels
			int_p[k+2] = (int)(rp_buf[array_length * 2 + j] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
			int_p[k+3] = (int)(rp_buf[array_length * 2 + j + 1] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);

			// Center channel
			int_p[k+4] = (int)(rp_buf[array_length * 4 + i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);

			// Subwoofer channel
			int_p[k+5] = (int)(rp_buf[array_length * 5 + i] * (float)MTH_32_BIT_REAL_CONVERSION_FACTOR);
		}
		break;

	default:
		/* Not a supported bit width */
		return(NOT_OKAY);
	}

	return(OKAY);
}