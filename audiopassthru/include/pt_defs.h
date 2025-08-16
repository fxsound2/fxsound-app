/*
FxSound
Copyright (C) 2025  FxSound LLC

Contributors:
	www.theremino.com (2025)

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
#ifndef _PT_DEFS_H_
#define _PT_DEFS_H_

#ifndef __ANDROID__
#include <windows.h>
#endif //WIN32

/* Maximum number of DSP/FX processors */
/* Defined to be 16 in Virtual Pack version 6.2 */
/* #define DSPFX_MAX_NUM_PROCS 16 */
/* Increased to 64 for Cakewalk SDK version */
#define DSPFX_MAX_NUM_PROCS 64

/* The number of different plug-in types */
#define DSPFX_NUM_PLUGIN_TYPES 32

/* Hardware oriented numeric constants */
#define TWO_PI 6.283185307

/* Definition of meter info structure */
struct hardwareMeterValType
{
	int  values_are_new;
	long left_in;
	long right_in;
	long left_out;
	long right_out;
	long dsp_status;
	float aux_vals[8]; /* For addition info transfer */
};

/* The following are the top registry locations */
#define DFX_RETAIL_REGISTRY_TOP        "SOFTWARE\\DFX"
#define DFX_RETAIL_REGISTRY_TOP_WIDE   L"SOFTWARE\\DFX"

#define DSPFX_RETAIL_REGISTRY_TOP      "SOFTWARE\\DSPFX Virtual Pack"
#define DSPFX_RETAIL_REGISTRY_TOP_WIDE L"SOFTWARE\\DSPFX Virtual Pack"

#define DSPFX_DEMO_REGISTRY_TOP        "SOFTWARE\\DSPFX Demo"
#define DSPFX_DEMO_REGISTRY_TOP_WIDE   L"SOFTWARE\\DSPFX Demo"

#define DFX_DEMO_REGISTRY_TOP          "SOFTWARE\\DFX Demo"
#define DFX_DEMO_REGISTRY_TOP_WIDE     L"SOFTWARE\\DFX Demo"


 /* Maximum fullpath string length */
#define PT_MAX_PATH_STRLEN             1024
#define PT_MAX_GENERIC_STRLEN          512

#endif /* _PT_DEFS_H */    
