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
#ifndef _DFX_KEY_VALUE_STORE_H_
#define _DFX_KEY_VALUE_STORE_H_

#include "codedefs.h"
#include "reg.h"

/*
 * Platform-independent settings storage used by dsp/ in place of direct
 * Windows Registry access. On Windows this delegates to the existing
 * regCreateKey_Wide/regReadKey_Wide/regRecursiveDeleteFolder_Wide helpers
 * (audiopassthru/src/reg) so on-disk key layout and behavior are unchanged.
 * On every other platform it's backed by a flat settings file.
 *
 * i_root is one of REG_CURRENT_USER / REG_LOCAL_MACHINE (see reg.h) and is
 * kept only to namespace entries the same way the registry did; it has no
 * per-user-vs-machine-wide access-control meaning outside Windows.
 */

int PT_DECLSPEC dfxKvWriteString_Wide(int i_root, wchar_t *wcp_full_key_path, wchar_t *wcp_value);
int PT_DECLSPEC dfxKvReadString_Wide(int i_root, wchar_t *wcp_full_key_path, int *ip_key_exists,
												  wchar_t *wcp_value_out, unsigned long ul_buffer_len);
int PT_DECLSPEC dfxKvRecursiveDeleteFolder_Wide(int i_root, wchar_t *wcp_folder_path);

#endif //_DFX_KEY_VALUE_STORE_H_
