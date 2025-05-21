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
#include <stdlib.h> 
#include <stdio.h>

#include "slout.h"
 
/*
 * FUNCTION: sloutDisplayFileAndLine()
 * DESCRIPTION:
 */
int sloutDisplayFileAndLine(char *cp_filename, long lp_line_num)
{
	return(NOT_OKAY);
}

/*
 * FUNCTION: CSlout()
 * DESCRIPTION: Contructor.  Initialize linenum to 1.
 */
CSlout::CSlout()
{
   m_linenum = 1;
}

/*
 * FUNCTION: Display()
 * DESCRIPTION: This is the default display function which will usually
 *              be overriden by the derived class.
 */
int CSlout::Display(int i_linenum, char *cp_msg)
{ 
   return(OKAY);      
}

/*
 * FUNCTION: Error()
 * DESCRIPTION: Construct an error message and pass it to display function.
 */
int CSlout::Error(int i_linetype, char *cp_msg)
{
   if (i_linetype == FIRST_LINE)
   {
      sprintf(m_msg, "Error: %s", cp_msg);
      m_linenum = 1;
   }
   else
   {
      sprintf(m_msg, "   %s", cp_msg);
      m_linenum++;
   }
   
   Display(m_linenum, m_msg);
     
   return(OKAY);      
} 
  
/*
 * FUNCTION: Warning()
 * DESCRIPTION: Construct a warning and pass it to display function.
 */
int CSlout::Warning(int i_linetype, char *cp_msg)
{
   if (i_linetype == FIRST_LINE)
   {
      sprintf(m_msg, "Warning: %s", cp_msg);
      m_linenum = 1;
   }
   else
   {
      sprintf(m_msg, "   %s", cp_msg);
      m_linenum++;
   }
   
   Display(m_linenum, m_msg);
     
   return(OKAY);      
}   
  
/*
 * FUNCTION: Message()
 * DESCRIPTION: Construct a message and pass it to display function.
 */
int CSlout::Message(int i_linetype, char *cp_msg)
{
   if (i_linetype == FIRST_LINE)
      m_linenum = 1;
   else 
      m_linenum++;
   
   sprintf(m_msg, "%s", cp_msg);
   Display(m_linenum, m_msg);
     
   return(OKAY);      
}

/*
 * FUNCTION: Display_Wide()
 * DESCRIPTION: This is the default display function which will usually
 *              be overriden by the derived class.
 */
int CSlout::Display_Wide(int i_linenum, wchar_t *wcp_msg)
{ 
   return(OKAY);      
}

#if defined( WIN32 ) // Wide char functions only supported in WIN32 builds.

/*
 * FUNCTION: Error_Wide()
 * DESCRIPTION: Construct an error message and pass it to display function.
 */
int CSlout::Error_Wide(int i_linetype, wchar_t *wcp_msg)
{
   if (i_linetype == FIRST_LINE)
   {
      swprintf(m_wcp_msg, L"Error: %s", wcp_msg);
      m_linenum = 1;
   }
   else
   {
      swprintf(m_wcp_msg, L"   %s", wcp_msg);
      m_linenum++;
   }
   
   Display_Wide(m_linenum, m_wcp_msg);
     
   return(OKAY);      
} 
  
/*
 * FUNCTION: Warning_Wide()
 * DESCRIPTION: Construct a warning and pass it to display function.
 */
int CSlout::Warning_Wide(int i_linetype, wchar_t *wcp_msg)
{
   if (i_linetype == FIRST_LINE)
   {
      swprintf(m_wcp_msg, L"Warning: %s", wcp_msg);
      m_linenum = 1;
   }
   else
   {
      swprintf(m_wcp_msg, L"   %s", wcp_msg);
      m_linenum++;
   }
   
   Display_Wide(m_linenum, wcp_msg);
     
   return(OKAY);      
}   
  
/*
 * FUNCTION: Message_Wide()
 * DESCRIPTION: Construct a message and pass it to display function.
 */
int CSlout::Message_Wide(int i_linetype, wchar_t *wcp_msg)
{
   if (i_linetype == FIRST_LINE)
      m_linenum = 1;
   else 
      m_linenum++;
   
   swprintf(m_wcp_msg, L"%s", wcp_msg);
   Display_Wide(m_linenum, m_wcp_msg);
     
   return(OKAY);      
}

#endif //Wide char functions only supported in WIN32 builds.
