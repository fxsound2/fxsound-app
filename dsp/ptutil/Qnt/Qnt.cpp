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
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> 

#include "mry.h"
#include "qnt.h"
#include "u_qnt.h"

/*
 * FUNCTION: qntFreeUp()
 * DESCRIPTION:
 *   Frees the passed qnt handle and sets to NULL.
 */
int PT_DECLSPEC qntFreeUp(PT_HANDLE **hpp_qnt)
{
	struct qntHdlType *cast_handle;

	cast_handle = (struct qntHdlType *)(*hpp_qnt);

	if (cast_handle == NULL)
		return(OKAY);

	/* Free the array */
	if (cast_handle->in_out_mode == QNT_INT_TO_INT)
	{
	   if (cast_handle->int_array != NULL)
	   {
	      free(cast_handle->int_array);
	   }
	}
	else if (cast_handle->in_out_mode == QNT_INT_TO_LONG)
	{
	   if (cast_handle->long_array != NULL)
	   {
	      free(cast_handle->long_array);
	   }
	}
	else if (cast_handle->in_out_mode == QNT_INT_TO_REAL)
	{
	   if (cast_handle->real_array != NULL)
	   {
	      free(cast_handle->real_array);
	   }
	}
	else if (cast_handle->in_out_mode == QNT_INT_TO_BOOST_CUT)
	{
	   if (cast_handle->filt_array != NULL)
	   {
	      free(cast_handle->filt_array);
	   }
	}

	free(cast_handle);

	*hpp_qnt = NULL;

	return(OKAY);
}

/*
 * FUNCTION: qntDump()
 * DESCRIPTION:
 *  Prints out handle to the screen.
 */
int PT_DECLSPEC qntDump(PT_HANDLE *hp_qnt)
{
	struct qntHdlType *cast_handle;
    int index;

	cast_handle = (struct qntHdlType *)hp_qnt;

	if (cast_handle == NULL)
		return(NOT_OKAY);
	
	cast_handle->slout_hdl->Message(FIRST_LINE, "QNT HDL:\n");	

    if (cast_handle->in_out_mode == QNT_INT_TO_INT)
    {
	   cast_handle->slout_hdl->Message(FIRST_LINE, "in_out_mode = QNT_INT_TO_INT\n");	
       cast_handle->slout_hdl->Message(FIRST_LINE, cast_handle->msg1);
       for (index = 0; index < cast_handle->array_size; index++)
       {
	      sprintf(cast_handle->msg1, "int_array[%d] = %d\n", index, 
	              cast_handle->int_array[index]);
          cast_handle->slout_hdl->Message(FIRST_LINE, cast_handle->msg1);         
       }
    } 
    
    if (cast_handle->in_out_mode == QNT_INT_TO_LONG)
    {
	   cast_handle->slout_hdl->Message(FIRST_LINE, "in_out_mode = QNT_INT_TO_LONG\n");	
       cast_handle->slout_hdl->Message(FIRST_LINE, cast_handle->msg1);
       for (index = 0; index < cast_handle->array_size; index++)
       {
	      sprintf(cast_handle->msg1, "long_array[%d] = %ld\n", index, 
	              cast_handle->long_array[index]);	
          cast_handle->slout_hdl->Message(FIRST_LINE, cast_handle->msg1);         
       }
    }
    
    if (cast_handle->in_out_mode == QNT_INT_TO_REAL)
    {
	   cast_handle->slout_hdl->Message(FIRST_LINE, "in_out_mode = QNT_INT_TO_REAL\n");	
       cast_handle->slout_hdl->Message(FIRST_LINE, cast_handle->msg1);
       for (index = 0; index < cast_handle->array_size; index++)
       {
	      sprintf(cast_handle->msg1, "real_array[%d] = %g\n", index, 
	              cast_handle->real_array[index]);	
          cast_handle->slout_hdl->Message(FIRST_LINE, cast_handle->msg1);         
       }
    }    

	return(OKAY);
}
