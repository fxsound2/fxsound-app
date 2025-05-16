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
#ifndef _DFX_SHARED_GLOBALS_H_
#define _DFX_SHARED_GLOBALS_H_

#include "codedefs.h"
#include "dfxpdefs.h"
#include "pt_defs.h"

struct dfxSharedGlobalsType
{
	// PTNOTE - any variables added or removed here need the initializations correctly set
	// in dfxShared.cpp . If the initializations are not set correctly the variables will not be created.

	realtype rp_meter_vals[DFXP_SPECTRUM_NUM_BANDS];
	unsigned long ul_msecs_processed; 
};

#endif //_DFX_SHARED_GLOBALS_H_