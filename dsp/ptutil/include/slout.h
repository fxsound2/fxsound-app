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
#ifndef _SLOUT_H_
#define _SLOUT_H_

#include "codedefs.h"

#define FIRST_LINE 1
#define NEXT_LINE  2
#define LAST_LINE  3

#define SLOUT_MAX_MSG_STRLEN 1024

/************************ 
 * Macro Definitions    *
 ************************/
// #define PT_RETURN_NOT_OKAY { sloutDisplayFileAndLine(__FILE__, __LINE__); return(NOT_OKAY); }
#define PT_RETURN_NOT_OKAY return(NOT_OKAY)

/************************ 
 * FUNCTIONS            *
 ************************/
int sloutDisplayFileAndLine(char *, long);

/************************ 
 * CLASSES              *
 ************************/
class PT_DECLSPEC CSlout {
public:
   CSlout(void);
   virtual int Display(int, char *);
   virtual int Message(int, char *);
   virtual int Error(int, char *);
   virtual int Warning(int, char *);

   virtual int Display_Wide(int, wchar_t *);

#if defined( WIN32 ) // Wide char functions only supported in WIN32 builds.
   virtual int Message_Wide(int, wchar_t *);
   virtual int Error_Wide(int, wchar_t *);
   virtual int Warning_Wide(int, wchar_t *);
#endif //WIN32

private:
   char m_msg[SLOUT_MAX_MSG_STRLEN];
   wchar_t m_wcp_msg[SLOUT_MAX_MSG_STRLEN];
   int m_linenum;
};

#endif //_SLOUT_H_
