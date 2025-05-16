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
#include "codedefs.h"

/* Standard includes */
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "slout.h"
#include "operatingSystem.h"
#include "reg.h"
#include "pstr.h"

/*
 * FUNCTION: operatingSystemIsMouseButtonDown()
 * DESCRIPTION:
 * 
 * Checks if the passed mouse button is down.
 *   (OPERATION_SYSTEM_MOUSE_BUTTON_NONE, OPERATION_SYSTEM_MOUSE_BUTTON_LEFT, or OPERATION_SYSTEM_MOUSE_BUTTON_RIGHT)
 *
 * This function takes care of a windows bug which causes us to take into account if the user has 
 * swapped the mouse button for left handed mode.
 *
 */
int PT_DECLSPEC operatingSystemIsMouseButtonDown(int i_os_mouse_button_to_check, int *ip_is_down)
{
	int i_mouse_buttons_swapped;
	int i_vkey_to_check;

	*ip_is_down = IS_FALSE;

	/* Check if the mouse buttons have been swapped (left handed mode) */
	i_mouse_buttons_swapped = GetSystemMetrics(SM_SWAPBUTTON);

	if (i_os_mouse_button_to_check == OPERATION_SYSTEM_MOUSE_BUTTON_LEFT)
	{
		if (i_mouse_buttons_swapped)
			i_vkey_to_check =  VK_RBUTTON;
		else
			i_vkey_to_check =  VK_LBUTTON;
	}
	else
	{
		if (i_mouse_buttons_swapped)
			i_vkey_to_check =  VK_LBUTTON;
		else
			i_vkey_to_check =  VK_RBUTTON;
	}

   if (GetAsyncKeyState(i_vkey_to_check))
      *ip_is_down = IS_TRUE;

	return(OKAY);
}