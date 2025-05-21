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
#ifndef _U_PRELST_H_
#define _U_PRELST_H_

#include "slout.h"

/* Effects list handle definition */
struct prelstHdlType {
	/* Initialization info */
	CSlout *slout_hdl;
	char msg1[1024]; /* String for messages */ 
	wchar_t wcp_msg1[1024]; /* String for messages */ 

	wchar_t *wcp_factory_dir; /* Directory factory presets are in */
   wchar_t *wcp_user_dir; /* Directory user presets are in */

	/* Array of filenames */
	int user_min_index;
	int user_max_index;
	int *exist_flags;
};

#endif //_U_PRELST_H_