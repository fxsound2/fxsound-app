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
#ifndef _DSPFXP_H_
#define _DSPFXP_H_

typedef int DSPFXP_HANDLE;

/* Min and Max control values */
#define DSPFXP_CTRL_MIN 0.0
#define DSPFXP_CTRL_MAX 1.0

#define DSPFXP_SDK_OKAY     0
#define DSPFXP_SDK_NOT_OKAY 1

#define DSPFXP_SDK_FALSE 0
#define DSPFXP_SDK_TRUE  1

/* Trace message file (under system tmp folder) */
#define DSPFXP_MESSAGE_FILE "dspfxp_trace.txt"

#endif //_DSPFXP_H_
