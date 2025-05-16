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
#include <dos.h>
#include <conio.h>
#include <math.h>
#include <windows.h>

#include "codedefs.h"
#include "slout.h"
#include "pwav.h"
#include "u_pwav.h"

/*
 * FUNCTION: pwavCheckIfLegalSampFreq()
 * DESCRIPTION:
 *   Sets up and reads from the input.
 *
 */
int PT_DECLSPEC pwavCheckIfLegalSampFreq(long l_samp_freq, int *ip_legal_samp_freq)
{
	*ip_legal_samp_freq = IS_FALSE;

	if ((l_samp_freq == 8000L) ||
		 (l_samp_freq == 11025L) ||
       (l_samp_freq == 12000L) ||
       (l_samp_freq == 16000L) ||
       (l_samp_freq == 22050L) ||
       (l_samp_freq == 24000L) ||
       (l_samp_freq == 32000L) ||
       (l_samp_freq == 44100L) ||
       (l_samp_freq == 44056L) ||
       (l_samp_freq == 48000L))
	{
		*ip_legal_samp_freq = IS_TRUE;
	}

   return(OKAY);
}  

