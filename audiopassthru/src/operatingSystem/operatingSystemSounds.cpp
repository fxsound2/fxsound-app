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
 * FUNCTION: operatingSystemMuteSystemSounds()
 * DESCRIPTION:
 * 
 * Takes care of automatically muting and restoring the system sounds during recording so user does not
 * get a beep in his recording while changing the volume level.
 *
 * The i_run_mode is one of the following:
 * OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_APP_STARTUP		
 * OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_RECORDING_START	
 * OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_RECORDING_STOP
 *
 * The passed in wcp_user_app_reg_top_folder will be something like: "MaxRecorder\1\500"
 *
 * The logic is as follows:
 *
 * LOCAL_REG = HKEY_CURRENT_USER\MaxRecorder\1\500\SysSounds\.Current
 * SYS_REG = HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\.Default\.Current
 *
 * ----- RECORDING STARTS ------:
 * if (LOCAL_REG does not exist) || (LOCAL_REG == "restored")
 * {
 *     copy SYS_REG to LOCAL_REG;
 * }
 *
 * set SYS_REG to empty;
 *
 * ----- RECORDING STOPS -----:
 *
 * copy LOCAL_REG to SYS_REG
 * set LOCAL_REG to "restored"
 *
 *
 * ----- APP STARTUP -------:
 * Take care of case in which user changes system sound after crash:
 *
 * if ((LOCAL_REG exists) && (LOCAL_REG != "restored") && (SYS_REG == empty))
 *
 *  copy LOCAL_REG to SYS_REG
 *  set LOCAL_REG to "restored" 
 *
 */
int PT_DECLSPEC operatingSystemMuteSystemSoundsForRecording(int i_run_mode, wchar_t *wcp_user_app_reg_top_folder)
{
	int i_comparison;
	int i_comparison_is_system_key_value_empty;
	int i_local_key_exists_flag;
	int i_system_key_exists_flag;
	wchar_t wcp_local_full_key_path[PT_MAX_PATH_STRLEN];
	wchar_t wcp_local_key_value[PT_MAX_PATH_STRLEN];
	wchar_t wcp_system_key_value[PT_MAX_PATH_STRLEN];

	// Get Local reg value
	swprintf(wcp_local_full_key_path, L"%s\\%s", wcp_user_app_reg_top_folder, OPERATING_SYSTEM_MUTE_SOUNDS_LOCAL_REG_KEYNAME_WIDE);

	if (regReadKey_Wide(REG_CURRENT_USER, wcp_local_full_key_path, 
					    &i_local_key_exists_flag, wcp_local_key_value,
						PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	if (pstrRemoveTrailingSpaces_Wide(wcp_local_key_value) != OKAY)
		return(NOT_OKAY);

	// Get System reg value
	if (regReadKey_Wide(REG_CURRENT_USER, OPERATING_SYSTEM_MUTE_SOUNDS_SYSTEM_REGISTRY_PATH, 
					    &i_system_key_exists_flag, wcp_system_key_value,
						PT_MAX_PATH_STRLEN) != OKAY)
		return(NOT_OKAY);

	/* If for some reason the system value does not exist, do nothing */
	if (!i_system_key_exists_flag)
		return(OKAY);

	if (pstrRemoveTrailingSpaces_Wide(wcp_system_key_value) != OKAY)
		return(NOT_OKAY);

	if (i_run_mode == OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_APP_STARTUP)
	{
		if (pstrCompareStrings(wcp_local_key_value, OPERATING_SYSTEM_MUTE_SOUNDS_RESTORED_VALUE_WIDE, IS_TRUE, &i_comparison) != OKAY)
			return(NOT_OKAY);

		if (pstrCompareStrings(wcp_system_key_value, L"", IS_TRUE, &i_comparison_is_system_key_value_empty) != OKAY)
			return(NOT_OKAY);
		
		if (i_local_key_exists_flag == IS_TRUE && i_comparison != PSTR_COMPARE_STRINGS_EQUAL && i_comparison_is_system_key_value_empty == PSTR_COMPARE_STRINGS_EQUAL)
		{
			// Copy LOCAL REG VALUE to SYSTEM REG VALUE
			if (regCreateKey_Wide(REG_CURRENT_USER, OPERATING_SYSTEM_MUTE_SOUNDS_SYSTEM_REGISTRY_PATH, wcp_local_key_value) != OKAY)
				return(NOT_OKAY);

			// Set LOCAL REG VALUE to "restored"
			if (regCreateKey_Wide(REG_CURRENT_USER, wcp_local_full_key_path, OPERATING_SYSTEM_MUTE_SOUNDS_RESTORED_VALUE_WIDE) != OKAY)
				return(NOT_OKAY);
		}
	}

	if (i_run_mode == OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_RECORDING_START)
	{
		if (pstrCompareStrings(wcp_local_key_value, OPERATING_SYSTEM_MUTE_SOUNDS_RESTORED_VALUE_WIDE, IS_TRUE, &i_comparison) != OKAY)
			return(NOT_OKAY);
		
		if (i_local_key_exists_flag == IS_FALSE || i_comparison == PSTR_COMPARE_STRINGS_EQUAL)
		{
			// Copy SYSTEM REG VALUE to LOCAL REG VALUE
			if (regCreateKey_Wide(REG_CURRENT_USER, wcp_local_full_key_path, wcp_system_key_value) != OKAY)
				return(NOT_OKAY);
		}

		// Set SYSTEM REG VALUE TO empty
		if (regCreateKey_Wide(REG_CURRENT_USER, OPERATING_SYSTEM_MUTE_SOUNDS_SYSTEM_REGISTRY_PATH, L"") != OKAY)
			return(NOT_OKAY);
	}

	if (i_run_mode == OPERATING_SYSTEM_MUTE_SOUNDS_RUN_MODE_RECORDING_STOP)
	{
		// Copy LOCAL REG VALUE to SYSTEM REG VALUE
		if (regCreateKey_Wide(REG_CURRENT_USER, OPERATING_SYSTEM_MUTE_SOUNDS_SYSTEM_REGISTRY_PATH, wcp_local_key_value) != OKAY)
			return(NOT_OKAY);

		// Set LOCAL REG VALUE to "restored"
		if (regCreateKey_Wide(REG_CURRENT_USER, wcp_local_full_key_path, OPERATING_SYSTEM_MUTE_SOUNDS_RESTORED_VALUE_WIDE) != OKAY)
			return(NOT_OKAY);
	}

	return(OKAY);
}