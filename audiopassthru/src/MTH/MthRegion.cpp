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
#include "codedefs.h"
#include "mth.h"
#include "u_mth.h"

/*
 * FUNCTION: mthCheckIfRegion1WithinRegion2()
 * DESCRIPTION:
 *
 * Calculates and passes back whether on not region 1 is at all within region 2.
 *
 * If so, at least one corner from region1 will be inside region2
 * Test each corner of region1 to see if its in region2.
 *	
 */
int PT_DECLSPEC mthCheckIfRegion1WithinRegion2(int i_region1_xmin, int i_region1_ymin, 
											              int i_region1_xmax, int i_region1_ymax,
												           int i_region2_xmin, int i_region2_ymin, 
												           int i_region2_xmax, int i_region2_ymax,
															  int *ip_overlap)
{
	*ip_overlap = IS_TRUE;

	/* Test first corner: (i_region1_xmin, i_region1_ymin) */
	if ((i_region1_xmin >= i_region2_xmin) && (i_region1_xmin <= i_region2_xmax) && 
		 (i_region1_ymin >= i_region2_ymin) && (i_region1_ymin <= i_region2_ymax))
	{
	   *ip_overlap = IS_TRUE;
      return(OKAY);
	}

	/* Test second corner: (i_region1_xmax, i_region1_ymin) */
	if ((i_region1_xmax >= i_region2_xmin) && (i_region1_xmax <= i_region2_xmax) && 
		 (i_region1_ymin >= i_region2_ymin) && (i_region1_ymin <= i_region2_ymax))
	{
	   *ip_overlap = IS_TRUE;
      return(OKAY);
	}

	/* Test third corner: (i_region1_xmin, i_region1_ymax) */
	if ((i_region1_xmin >= i_region2_xmin) && (i_region1_xmin <= i_region2_xmax) && 
		 (i_region1_ymax >= i_region2_ymin) && (i_region1_ymax <= i_region2_ymax))
	{
	   *ip_overlap = IS_TRUE;
      return(OKAY);
	}

	/* Test fourth corner: (i_region1_xmax, i_region1_ymax) */
	if ((i_region1_xmax >= i_region2_xmin) && (i_region1_xmax <= i_region2_xmax) && 
		 (i_region1_ymax >= i_region2_ymin) && (i_region1_ymax <= i_region2_ymax))
	{
	   *ip_overlap = IS_TRUE;
      return(OKAY);
	}

	*ip_overlap = IS_FALSE;

   return(OKAY);
}

