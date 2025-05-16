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
 * FUNCTION: mthEncryptLong()
 * DESCRIPTION:
 *
 *  Uses a passed in key to both encrypt and decrypt the passed in
 *  long value. The same key is used both for encryption and decryption.
 *  The first call encrypts the value, and to decrypt the value,
 *  call the function again with the same key and the encrypted value.
 *  The key is a fixed unsigned long value.
 */
int PT_DECLSPEC mthEncryptLong(unsigned long ul_key, long l_inval, long *lp_encrypted) 
{
	int i;
	long scrambled;
	/* "Scramble" array. Insures that values with
	 * leading 1 in MSB still have that when recovered.
	 */
	int scramble[32] =   { 9, 24,  2,  7, 30, 18,  3, 22,
	 					  11, 29,  0, 12, 27,  5, 26, 19,
				  		  13,  1, 10,  8, 28, 15,  4, 23,
						  14, 25,  6, 16, 20, 17, 21, 31};

	/* First scramble the bits */
	scrambled = 0;
	for( i=0; i<32; i++)
	{
		if( l_inval & (1 << i) )
			scrambled |= (1 << scramble[i]);
	}

	*lp_encrypted = scrambled ^ ul_key;
    
    return(OKAY);
}

/*
 * FUNCTION: mthDecryptLong()
 * DESCRIPTION:
 *
 *  Uses a passed in key to both encrypt and decrypt the passed in
 *  long value. The same key is used both for encryption and decryption.
 *  The first call encrypts the value, and to decrypt the value,
 *  call the function again with the same key and the encrypted value.
 *  The key is a fixed unsigned long value.
 */
int PT_DECLSPEC mthDecryptLong(unsigned long ul_key, long l_inval, long *lp_decrypted) 
{
	int i;
	long unscrambled;
	/* "Unscramble" array. Insures that values with
	 * leading 1 in MSB still have that when recovered.
	 */
	int unscramble[32] = {10, 17,  2,  6, 22, 13, 26,  3,
	 					  19,  0, 18,  8, 11, 16, 24, 21,
						  27, 29,  5, 15, 28, 30,  7, 23,
						   1, 25, 14, 12, 20,  9,  4, 31};

	l_inval ^= ul_key;
    
	/* Unscramble the bits */
	unscrambled = 0;
	for( i=0; i<32; i++)
	{
		if( l_inval & (1 << i) )
			unscrambled |= (1 << unscramble[i]);
	}

	*lp_decrypted = unscrambled;

    return(OKAY);
}
