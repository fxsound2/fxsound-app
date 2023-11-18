/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: Dutcom.h
 * DATE: 7/7/97
 * AUTOHOR: Paul F. Titchener
 * DESCRIPTION: Macros that communicate directly with PC via
 * transfers, status and command registers.
 */

/* HARDWARE SPECIFIC MACROS */
/* Waits for a 32 bit value from PC.
 */
#define wait_for_val(val) \
{ \
	while (io_adr->comd_stat & 0x40) \
		; \
	val = io_adr->xfr_reg; \
}

/* If ready, gets a 32 bit value from PC.
 */
#define look_for_val(val) \
{ \
	if( !(io_adr->comd_stat & 0x40) ) \
	{ \
		val = io_adr->xfr_reg; \
	} \
}

/* Looks for and loads a parameter into DSP memory space.
 * Note - a 2 step process, takes two calls to get value into memory.
 * For DMA, there are calls at different locations, so statics must
 * be declared in common place.
 */
#if (defined(DUIO_B) | defined(DUIO_BA) | defined(DUIO_BD))
#define load_parameter() \
if( (*(volatile long *)(DSP_DMA_IN_TRANSFER)) == 0 ) \
{ \
	if( address_valid_flag_MACRO ) \
	{ \
		load_mem_value(parm_address_MACRO, address_valid_flag_MACRO); \
	} \
	else \
	{ \
		load_mem_address(parm_address_MACRO, address_valid_flag_MACRO); \
	} \
}
#elif (defined(DUIO_A) | defined(DUIO_D))
#define load_parameter() \
{ \
	static long parm_address_MACRO; \
	static long address_valid_flag_MACRO = 0; \
	if( address_valid_flag_MACRO ) \
	{ \
		load_mem_value(parm_address_MACRO, address_valid_flag_MACRO); \
	} \
	else \
	{ \
		load_mem_address(parm_address_MACRO, address_valid_flag_MACRO); \
	} \
}
#else
#define load_parameter() ;
#endif

/* This is Nils New Method */
#if (defined(DUIO_B) | defined(DUIO_BA) | defined(DUIO_BD) | defined(DUIO_A) | defined(DUIO_D)) 

volatile unsigned iflg;

/* First part of 2 step process to load a parameter from PC into DSP memory */
#define load_mem_address(parm_address, address_flag) \
{ \
	asm("	STI	IOF,@_iflg"); \
	if ( ! (iflg & 0x08) ) \
	{ \
		parm_address = io_adr->xfr_reg; \
		address_flag = 1; \
	} \
}                       

/* Second step in process to load value from PC into DSP memory */
#define load_mem_value(parm_address, address_flag) \
{ \
	asm("	STI	IOF,@_iflg"); \
	if ( ! (iflg & 0x08) ) \
	{ \
		long parm_MACRO; \
		parm_MACRO = io_adr->xfr_reg; \
		*(volatile long *)parm_address = parm_MACRO; \
		address_flag = 0; \
	} \
}
#endif


/* Below is Nil's older method */
#ifdef NOT_DEFINED_NILS
/* First part of 2 step process to load a parameter from PC into DSP memory */
#define load_mem_address(parm_address, address_flag) \
{ \
	if( !(io_adr->comd_stat & 0x40) ) \
	{ \
		parm_address = io_adr->xfr_reg; \
		address_flag = 1; \
	} \
}                       

/* Second step in process to load value from PC into DSP memory */
#define load_mem_value(parm_address, address_flag) \
{ \
	if( !(io_adr->comd_stat & 0x40) ) \
	{ \
		long parm_MACRO; \
		parm_MACRO = io_adr->xfr_reg; \
		*(volatile long *)parm_address = parm_MACRO; \
		address_flag = 0; \
	} \
}
#endif /* NOT_DEFINED_NILS */

/* Writes value to PC transfer reg, only if reg is empty */
#define write_toPC(i_val) \
{ \
	if( !(io_adr->comd_stat & 0x80) ) \
		io_adr->xfr_reg = i_val; \
}

/* Writes data series value to PC transfer reg, only if reg is empty
 * increments state of data series if write was successful
 */
#define write_series_toPC(i_val, state) \
{ \
	if( !(io_adr->comd_stat & 0x80) ) \
	{ \
		io_adr->xfr_reg = i_val; \
		state++; \
	} \
}

/* Writes value to PC transfer reg, only if reg is empty,
 * Used for last data point in series, automatically resets state
 */
#define write_last_series_toPC(i_val, state) \
{ \
	if( !(io_adr->comd_stat & 0x80) ) \
	{ \
		io_adr->xfr_reg = i_val; \
		state = 0; \
	} \
} \

/* Writes meters and status value transfer reg.
 * Measures 24 cycles usually, with an occasional 16 (PC not ready?)
 */  
#if (defined(DUIO_B) | defined(DUIO_BA) | defined(DUIO_BD) | defined(DSPSOFT_TARGET))
#define write_meters_and_status(imeter1, imeter2, ometer1, ometer2, status, state) ;
#else
/* Nil's new version that allows running with serial card */
#define write_meters_and_status(imeter1, imeter2, ometer1, ometer2, status, state) \
{ \
  asm("	STI	IF,@_iflg"); \
  if (iflg & 0x02) { \
  asm("	ANDN	2,IF"); \
  switch( state ) \
  { \
   case LEFT_IN_STATE: \
    io_adr->xfr_reg = imeter1; \
    state++; \
        break; \
   case RIGHT_IN_STATE: \
    io_adr->xfr_reg = imeter2; \
    state++; \
    break; \
   case LEFT_OUT_STATE: \
    io_adr->xfr_reg = ometer1; \
    state++; \
    break; \
   case RIGHT_OUT_STATE: \
    io_adr->xfr_reg = ometer2; \
    state++; \
        break; \
   case STATUS_STATE: \
    io_adr->xfr_reg = status; \
    state = 0; \
    status = 0; \
        break; \
  } \
 } \
}
#ifdef NILS_NOT_DEFINED
/* Original version that used status reg bit check */
#define write_meters_and_status(imeter1, imeter2, ometer1, ometer2, status, state) \
{ \
 if( !(io_adr->comd_stat & 0x80) ) \
  switch( state ) \
  { \
   case LEFT_IN_STATE: \
    io_adr->xfr_reg = imeter1; \
    state++; \
	break; \
   case RIGHT_IN_STATE: \
    io_adr->xfr_reg = imeter2; \
    state++; \
    break; \
   case LEFT_OUT_STATE: \
    io_adr->xfr_reg = ometer1; \
    state++; \
    break; \
   case RIGHT_OUT_STATE: \
    io_adr->xfr_reg = ometer2; \
    state++; \
	break; \
   case STATUS_STATE: \
    io_adr->xfr_reg = status; \
    state = 0; \
    status = 0; \
	break; \
  } \
}
#endif /* NILS_NOT_DEFINED */
#endif /* DUIO_B|DUIO_BA|DUIO_BD */

/* For sending back averaged meter data in PC case.
 * Writes averaged meter data temporarily as a float.
 * Will be converted to long and factored in calling function.
 */
#if defined(DSPSOFT_TARGET) & (PT_DSP_BUILD == PT_DSP_DSPFX)
#define write_meter_average() \
*(float *)&(sp_meters->left_in) = in_meter1_dma; \
*(float *)&(sp_meters->right_in) = in_meter2_dma; \
*(float *)&(sp_meters->left_out) = out_meter1_dma; \
*(float *)&(sp_meters->right_out) = out_meter2_dma; \
sp_meters->dsp_status = status; \
sp_meters->values_are_new = 1;
#else
#define write_meter_average() ;
#endif

/* OLD VERSION - Writes status value transfer reg, only if reg is empty.
 * Used if status is the last data point in series, automatically resets state,
 * and status is reset (zeroed) after successfully sent.
 */
#define write_status(i_val, state) \
{ \
	if( !(io_adr->comd_stat & 0x80) ) \
	{ \
		io_adr->xfr_reg = i_val; \
		state = 0; \
		i_val = 0; \
	} \
} \

/* Sets DSP to HOST Flag Bit */
#define SetHostFlag() \
io_adr->comd_stat = command0_shadow = command0_shadow | 0x08;

/* Clears DSP to HOST Flag Bit */
#define ClearHostFlag() \
io_adr->comd_stat = command0_shadow = command0_shadow & ~0x08;

/* Macro to update input meters */
#define update_input_meters(i1, i2, state) \
if(state == LEFT_IN_STATE) \
{ \
  write_series_toPC(i1, state); \
} \
else \
{ \
  if(state == RIGHT_IN_STATE) \
  { \
    write_series_toPC(i2, state); \
  } \
}

/* Macro to update output meters */
#define update_output_meters(i1, i2, state) \
{ \
	if(state == LEFT_OUT_STATE) \
	{ \
	  write_series_toPC(i1, state); \
	} \
	else \
	{ \
	   if(state == RIGHT_OUT_STATE) \
	   { \
	      write_series_toPC(i2, state); \
	   } \
	} \
}
