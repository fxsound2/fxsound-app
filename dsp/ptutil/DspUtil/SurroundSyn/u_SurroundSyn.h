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
#ifndef _U_SURROUND_SYN_H_
#define _U_SURROUND_SYN_H_

#include "SurroundSyn.h"

#define SURROUND_SYN_BIAS 1.0e-5

#define SURROUND_SYN_NLPZEROS 2
#define SURROUND_SYN_NLPPOLES 2
#define SURROUND_SYN_LPGAIN   (1.0/1.384982154e+04)

#define SURROUND_SYN_FRONT_MAIN 0.75
#define SURROUND_SYN_FRONT_BLEED 0.25
#define SURROUND_SYN_CENTER_GAIN 0.75
#define SURROUND_SYN_REAR_GAIN 0.5
#define SURROUND_SYN_REAR_DIFF 0.8

// Configuration Handle definition
struct SurroundSynHdlType
{
	// For LPE channel low pass filtering
	float xv[SURROUND_SYN_NLPZEROS+1];
	float yv[SURROUND_SYN_NLPPOLES+1];
};

// Local functions

#endif // _U_SURROUND_SYN_H_