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
#include <stdlib.h>
#include <stdio.h>

#include "codedefs.h"
#include "u_BinauralSyn.h"

/*
 * FUNCTION: BinauralSynGetNumCoeffs()
 * DESCRIPTION:
 *  Gets the number of coefficients used in each channel filter.
 */
int PT_DECLSPEC BinauralSynGetNumCoeffs(PT_HANDLE *hp_BinauralSyn, int *ip_num_coeffs)
{
	struct BinauralSynHdlType *cast_handle;

	cast_handle = (struct BinauralSynHdlType *)(hp_BinauralSyn);
 
	if (cast_handle == NULL)
		return(NOT_OKAY);

	*ip_num_coeffs = cast_handle->num_coeffs;

	return(OKAY);
}
