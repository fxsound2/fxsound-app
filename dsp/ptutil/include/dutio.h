/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/* 
 * FILE: Dutio.h
 * DATE: 7/7/97
 * AUTOHOR: Paul F. Titchener
 * DESCRIPTION: Macros to run A/D and D/A converters, AES
 * This version had mods to run AES transmitter from receiver clock
 */
/* COMMON I/O MACROS */
/*
 * MACRO: dutilGetInputsAIO()
 * DESCRIPTION: Get Analog Inputs 1 and 2 (Left and Right)
 * Note that this input function uses the MSB's to hold the value.  This
 * causes the signal to be clipped automatically when assigned from a
 * float back to a long if the float value is out of range of the long.
 * float representation of max long is 2.1474836e9
 * Max positive long is 2147483647 . Max negative int appears to be -2147483647
 * when assigned from a more negative float.
 */
/* Faster way is to rotate number and use carry detect, as in
 * Ariel example.  Can it be done smoothly in C???
 * Shifts on i_val1 are being done to extend sign- there's
 * probably a faster way.
 */
/* NOTE- BEWARE OF NESTED {}'s WITH if and else !!! */
/* is_input_ready() macro takes ? cycles 
 * if section takes 16 cycles up to update_input_meters.
 * up_date_input_meters takes 12 or 24 cycles- probably 24 when writing. 
 * For else false (mono), test and real assign takes 12
 * -> Max total cycles > 52 + is_input_ready test.
 */
                                                              
#ifdef DUIO_D /* AES VERSION - is_output_ready() is defined to NULL */
#define dutilGetInputsAndMeter(r1, r2, imeter1, imeter2, ometer1, ometer2) \
if( *(volatile long *)(DSP_STEREO_IN_FLAG) ) \
{ \
  is_input_ready(); \
  imeter2 = ser_adr->rcv_data; \
  is_output_ready(); \
  ser_adr->xmit_data = ometer2; \
  r1 = imeter1; \
  r2 = imeter2; \
} \
else \
{ \
  int dummy; \
  is_input_ready(); \
  dummy = ser_adr->rcv_data; \
  imeter2 = 0L; \
  is_output_ready(); \
  ser_adr->xmit_data = ometer2; \
  r1 = r2 = imeter1; \
}
#endif /* DUIO_D */

#ifdef DUIO_A 
/* Pure analog version */
#define dutilGetInputsAndMeter(r1, r2, imeter1, imeter2, ometer1, ometer2) \
is_input_ready(); \
if( *(volatile long *)(DSP_STEREO_IN_FLAG) ) \
{ \
  imeter1 = *(volatile unsigned long *) AD_DATA_REG; \
  imeter2 = imeter1; \
  imeter1 &= 0xFFFF0000; \
  imeter2 <<= 16; \
  r1 = imeter1; \
  r2 = imeter2; \
} \
else \
{ \
  imeter1 = *(volatile unsigned long *) AD_DATA_REG; \
  imeter1 &= 0xFFFF0000; \
  r1 = r2 = imeter1; \
  imeter2 = 0L; \
}
#endif

/* Version for buffered on DSP. May need adjustment for
 * a 32 bit words
 */
#if defined(DUIO_B)
/* Pure buffered i/o version (wave, saw track only, direct x track only */
#define dutilGetInputsAndMeter(r1, r2, imeter1, imeter2, ometer1, ometer2) \
if( *(volatile long *)(DSP_STEREO_IN_FLAG) ) \
{ \
  imeter1 = read_in_buf[data_index]; \
  imeter2 = imeter1; \
  imeter2 &= 0xFFFF0000; \
  in_meter2_dma += labs(imeter2 >> 16); \
  imeter1 <<= 16; \
  in_meter1_dma += labs(imeter1 >> 16); \
  r1 = (float)imeter1; \
  r2 = (float)imeter2; \
} \
else \
{ \
  imeter1 = read_in_buf[data_index]; \
  imeter1 <<= 16; \
  in_meter1_dma += labs(imeter1 >> 16); \
  r1 = r2 = (float)imeter1; \
  imeter2 = 0L; \
}
#endif

/* PC DSPFX software version (wave, saw track only, direct x track only */
/* First part of outer if handles 32 bit floats, second 16 ints */
#if defined(DSPSOFT_TARGET)
#if (PT_DSP_BUILD == PT_DSP_DSPFX)
#if defined(DSPSOFT_32_BIT)
#define dutilGetInputsAndMeter(in1, in2, status)\
if( *(volatile long *)(DSP_STEREO_IN_FLAG) )\
{\
  float r_tmp;\
  in1 = ((float *)read_in_buf)[data_index];\
  r_tmp = fabs(in1);\
  if( r_tmp >= (float)(32767.0/32768.0) )\
	status |= IN1_CLIP;\
  in_meter1_dma += r_tmp;\
  in2 = ((float *)read_in_buf)[data_index + 1];\
  r_tmp = fabs(in2);\
  if( r_tmp >= (float)(32767.0/32768.0) )\
	status |= IN2_CLIP;\
  in_meter2_dma += r_tmp;\
}\
else\
{ \
  float r_tmp;\
  in1 = in2 = ((float *)read_in_buf)[data_index];\
  r_tmp = fabs(in1);\
  if( r_tmp >= (float)(32767.0/32768.0) )\
	status |= IN1_CLIP;\
  in_meter1_dma += r_tmp;\
}
#else /* 16 bit I/O case */
#define dutilGetInputsAndMeter(in1, in2, status)\
if( *(volatile long *)(DSP_STEREO_IN_FLAG) )\
{\
  float r_tmp;\
  short int *short_in = (short int *)&(read_in_buf[data_index]);\
  in1 = (float)*short_in * (float)(1.0/32768.0);\
  r_tmp = fabs(in1);\
  if( r_tmp >= (float)(32767.0/32768.0) )\
	status |= IN1_CLIP;\
  in_meter1_dma += r_tmp;\
  in2 = (float)*(++short_in) * (float)(1.0/32768.0);\
  r_tmp = fabs(in2);\
  if( r_tmp >= (float)(32767.0/32768.0) )\
	status |= IN2_CLIP;\
  in_meter2_dma += r_tmp;\
}\
else\
{\
  float r_tmp;\
  short int *short_in_buf = (short int *)read_in_buf;\
  in1 = in2 = (float)short_in_buf[data_index] * (float)(1.0/32768.0);\
  r_tmp = fabs(in1);\
  if( r_tmp >= (float)(32767.0/32768.0) )\
	status |= IN1_CLIP;\
  in_meter1_dma += r_tmp;\
}
#endif
#endif
#endif /* DSPSOFT_TARGET, DSPFX CASE */

/* PC DFX software version  */
/* Only need to handle 32 bit float case. */
#if defined(DSPSOFT_TARGET)
#if (PT_DSP_BUILD == PT_DSP_DFX)
#define dutilGetInputsAndMeter(in1, in2, status)\
if( *(volatile long *)(DSP_STEREO_IN_FLAG) )\
{\
  in1 = ((float *)read_in_buf)[data_index];\
  in2 = ((float *)read_in_buf)[data_index + 1];\
}\
else\
{ \
  in1 = in2 = ((float *)read_in_buf)[data_index];\
}
#endif
#endif /* DSPSOFT_TARGET, DFX CASE */

#ifdef DUIO_BA
/* Pure buffered signal in, analog out version (sound card, wave process direct)
 * Input read is done to sync to A/D clock
 */
#define dutilGetInputsAndMeter(r1, r2, imeter1, imeter2, ometer1, ometer2) \
is_input_ready(); \
imeter1 = *(volatile unsigned long *) AD_DATA_REG; \
if( *(volatile long *)(DSP_STEREO_IN_FLAG) ) \
{ \
  imeter1 = read_in_buf[data_index]; \
  imeter2 = imeter1; \
  imeter2 &= 0xFFFF0000; \
  in_meter2_dma += labs(imeter2 >> 16); \
  imeter1 <<= 16; \
  in_meter1_dma += labs(imeter1 >> 16); \
  r1 = imeter1; \
  r2 = imeter2; \
} \
else \
{ \
  imeter1 = read_in_buf[data_index]; \
  imeter1 <<= 16; \
  in_meter1_dma += labs(imeter1 >> 16); \
  r1 = r2 = imeter1; \
  imeter2 = 0L; \
}
#endif

#ifdef DUIO_D /* AES VERSION - is_output_ready() macro is defined NULL for AES */
#define dutilGet2ndAESInputOutput(imeter1, imeter2, ometer1, ometer2) \
is_input_ready(); \
imeter1 = ser_adr->rcv_data; \
is_output_ready(); \
ser_adr->xmit_data = ometer1;           
#endif

#if (defined(DUIO_A) | defined(DUIO_B) | defined(DUIO_BA) | defined(DSPSOFT_TARGET))
#define dutilGet2ndAESInputOutput(imeter1, imeter2, ometer1, ometer2) ;
#endif

/* Mutes inputs via PC value */
#if defined(DSPSOFT_TARGET) & (PT_DSP_BUILD == PT_DSP_DFX)
#define dutilMuteInputs(in1, in2) ;
#else
#define dutilMuteInputs(in1, in2) \
{ \
	float mute_val_MACRO = *(volatile float *)(DSP_MUTE_IN_FLAG); \
	in1 *= mute_val_MACRO; \
	in2 *= mute_val_MACRO; \
}
#endif

/* Waits for data to be available in input and output registers */   
#ifndef INPUT_WAIT_COUNT
#define INPUT_WAIT_COUNT
#endif
#ifndef OUTPUT_WAIT_COUNT
#define OUTPUT_WAIT_COUNT
#endif
#ifndef BM
#ifdef DUIO_D
#define is_input_ready() \
	 while( ((*(volatile unsigned long *) AD_CNTRL_REG)&0x0001) == 0) INPUT_WAIT_COUNT;
#define is_output_ready() ; /* Currently assumming output frame sync matches input */
#endif
#if (defined(DUIO_A) | defined(DUIO_BA))
#define is_input_ready() \
	 while( ((*(volatile unsigned long *) AD_CNTRL_REG)&0x0001) == 0) INPUT_WAIT_COUNT;
#define is_output_ready() \
	 while( ((*(volatile unsigned long *) AD_CNTRL_REG)&0x0002) == 0) OUTPUT_WAIT_COUNT;
#endif
#if (defined(DUIO_B) | defined(DUIO_BA))
#define is_input_ready() \
	 while( ((*(volatile unsigned long *) AD_CNTRL_REG)&0x0001) == 0) INPUT_WAIT_COUNT;
#endif	 
#else /* BM (benchmarking) is defined */
#define is_input_ready() ;
#define is_output_ready() ;
#endif

/* Forces data out without checking transmit status bit,
 * apparently works only if also doing reads
 */
/* Without left and right shifts on i_val1, got wrap around on positive clip */
#define dutilPutOutputs(r_val1, r_val2) \
{ \
	 long int i_val1_MACRO, i_val2_MACRO; \
	 i_val1_MACRO = r_val1; \
	 i_val1_MACRO >>= 16; \
	 i_val1_MACRO <<= 16; \
	 i_val2_MACRO = r_val2; \
	 i_val2_MACRO >>= 16; \
	 i_val2_MACRO &= 0x0000FFFF; \
	 i_val1_MACRO |= i_val2_MACRO; \
	 *(volatile unsigned long *) DA_DATA_REG = i_val1_MACRO; \
}

/* Note- based on compiler option, float to int conversion can
 * take up to 8 cycles for full precision (i_val1_MACRO = r_val1)
 * -mc option is faster, but towards neg infinity instead of zero
 */
#ifdef DUIO_D 
#define dutilPutOutputsAndMeter(r_val1, r_val2, ometer1, ometer2) \
{ \
	 ometer1 = r_val1; \
	 ometer2 = r_val2; \
}
#endif /* DUIO_D */

#ifdef DUIO_A
#define dutilPutOutputsAndMeter(r_val1, r_val2, ometer1, ometer2) \
{ \
	 long int i_val1_MACRO, i_val2_MACRO; \
	 ometer1 = i_val1_MACRO = r_val1; \
	 ometer2 = i_val2_MACRO = r_val2; \
	 i_val1_MACRO >>= 16; \
	 i_val1_MACRO <<= 16; \
	 i_val2_MACRO >>= 16; \
	 i_val2_MACRO &= 0x0000FFFF; \
	 i_val1_MACRO |= i_val2_MACRO; \
	 *(volatile unsigned long *) DA_DATA_REG = i_val1_MACRO; \
}
#endif /* DUIO_A */

/* Version for DSP based wave, saw and active x */
#if defined(DUIO_B)
#define dutilPutOutputsAndMeter(r_val1, r_val2, ometer1, ometer2) \
{ \
	 long int i_val1_MACRO, i_val2_MACRO; \
	 ometer1 = i_val1_MACRO = (long)r_val1; \
	 ometer2 = i_val2_MACRO = (long)r_val2; \
	 i_val2_MACRO >>= 16; \
  	 out_meter2_dma += labs(i_val2_MACRO); \
	 i_val2_MACRO <<= 16; \
	 i_val1_MACRO >>= 16; \
  	 out_meter1_dma += labs(i_val1_MACRO); \
	 i_val1_MACRO &= 0x0000FFFF; \
	 i_val2_MACRO |= i_val1_MACRO; \
	 read_out_buf[data_index] = i_val2_MACRO; \
	 if( data_index < (unsigned)(dma_buf_size)) \
	  	data_index++; \
}
#endif /* DUIO_B */

/* PC based version, DSPFX CASE. Note that this version cannot
 * reset data_index  */
/* Note that first case handles 32 float, next is 16 fixed */
/* Note- up to 4.0 version, scaled mono by 0.5 */
#if defined(DSPSOFT_TARGET) & (PT_DSP_BUILD == PT_DSP_DSPFX)
#if defined(DSPSOFT_32_BIT)
#define dutilPutOutputsAndMeter(r_val1, r_val2, status)\
if( *(volatile long *)(DSP_STEREO_IN_FLAG) )\
{\
	 float r_tmp;\
	 ((float *)read_out_buf)[data_index++] = r_val1;\
	 r_tmp = fabs(r_val1);\
	 if( r_tmp >= (float)(1.0) )\
		 status |= OUT1_CLIP;\
  	 out_meter1_dma += r_tmp;\
	 ((float *)read_out_buf)[data_index++] = r_val2;\
	 r_tmp = fabs(r_val2);\
	 if( r_tmp >= (float)(1.0) )\
		 status |= OUT2_CLIP;\
  	 out_meter2_dma += r_tmp;\
}\
else\
{\
	 float r_tmp;\
	 r_val1 = (r_val1 + r_val2);\
	 ((float *)read_out_buf)[data_index++] = r_val1;\
	 r_tmp = fabs(r_val1);\
	 if( r_tmp >= (float)(1.0) )\
		 status |= OUT1_CLIP;\
  	 out_meter1_dma += r_tmp;\
}
#else /* 16 bit I/O */
#define dutilPutOutputsAndMeter(r_val1, r_val2, status)\
if( *(volatile long *)(DSP_STEREO_IN_FLAG) )\
{\
	 float r_tmp;\
	 short int *short_out = (short int *)&(read_out_buf[data_index]);\
	 r_tmp = fabs(r_val1);\
	 if( r_tmp >= (float)(32767.0/32768.0) )\
	 {\
		 status |= OUT1_CLIP;\
		 if( r_val1 >= (float)(32767.0/32768.0) )\
			 r_val1 = (float)(32767.0/32768.0);\
		 else\
			 r_val1 = -(float)(32767.0/32768.0);\
	 }\
  	 out_meter1_dma += r_tmp;\
	 *short_out = (short int)(r_val1 * (float)(32768.0));\
	 r_tmp = fabs(r_val2);\
	 if( r_tmp >= (float)(32767.0/32768.0) )\
	 {\
		 status |= OUT2_CLIP;\
		 if( r_val2 >= (float)(32767.0/32768.0) )\
			 r_val2 = (float)(32767.0/32768.0);\
		 else\
			 r_val2 = -(float)(32767.0/32768.0);\
	 }\
  	 out_meter2_dma += r_tmp;\
	 *(++short_out) = (short int)(r_val2 * (float)(32768.0));\
	 data_index++;\
}\
else\
{\
	 float r_tmp;\
	 r_val1 = (r_val1 + r_val2);\
 	 r_tmp = fabs(r_val1);\
	 if( r_tmp >= (float)(32767.0/32768.0) )\
	 {\
		 status |= OUT1_CLIP;\
		 if( r_val1 >= (float)(32767.0/32768.0) )\
			 r_val1 = (float)(32767.0/32768.0);\
		 else\
			 r_val1 = -(float)(32767.0/32768.0);\
	 }\
  	 out_meter1_dma += r_tmp;\
	 ((short int *)read_out_buf)[data_index] = (short int)(r_val1 * (float)(32768.0));\
	 data_index++;\
}
#endif
#endif /* DSPSOFT_TARGET, DSPFX CASE */

/* PC based version, DFX CASE. Note that this version cannot
 * reset data_index  */
/* Note that we only need to handle 32 float case. */
#if defined(DSPSOFT_TARGET) & (PT_DSP_BUILD == PT_DSP_DFX)
#define dutilPutOutputsAndMeter(r_val1, r_val2, status)\
if( *(volatile long *)(DSP_STEREO_IN_FLAG) )\
{\
	 ((float *)read_out_buf)[data_index++] = r_val1;\
	 ((float *)read_out_buf)[data_index++] = r_val2;\
}\
else\
{\
	 r_val1 = (r_val1 + r_val2);\
	 ((float *)read_out_buf)[data_index++] = r_val1;\
}
#endif /* DSPSOFT_TARGET, DFX CASE */

#ifdef DUIO_BA
/* Note- Wave file format regards upper 16 bits of long as right
 * channel(in2) and lower 16 bits as left channel (in1). This
 * is reverse of what we need to play them on the DSP/FX codec,
 * so they are reversed in this macro.
 */
#define dutilPutOutputsAndMeter(r_val1, r_val2, ometer1, ometer2) \
{ \
	 long int i_val1_MACRO, i_val2_MACRO; \
	 ometer1 = i_val1_MACRO = r_val1; \
	 ometer2 = i_val2_MACRO = r_val2; \
	 i_val1_MACRO >>= 16; \
  	 out_meter1_dma += labs(i_val1_MACRO); \
	 i_val1_MACRO <<= 16; \
	 i_val2_MACRO >>= 16; \
  	 out_meter2_dma += labs(i_val2_MACRO); \
	 i_val2_MACRO &= 0x0000FFFF; \
	 i_val1_MACRO |= i_val2_MACRO; \
	 *(volatile unsigned long *) DA_DATA_REG = i_val1_MACRO; \
	 if( data_index < (dma_buf_size)) \
	  	data_index++; \
}
#endif /* DUIO_BA */

/* Clipping all handled in input and output routines */
#if defined(DSPSOFT_TARGET)
#define dutilSetClipStatus(in1, in2, out1, out2, status) ;
#endif

/* DSP case */
#if defined(DSP_TARGET)
#define dutilSetClipStatus(in1, in2, out1, out2, status) \
{ \
	  if( fabs(in1) > DSP_FLOAT_OUTPUT_CLIP_LEVEL ) \
		 status |= IN1_CLIP; \
	  if( fabs(in2) > DSP_FLOAT_OUTPUT_CLIP_LEVEL ) \
		 status |= IN2_CLIP; \
	  if( fabs(out1) > DSP_FLOAT_OUTPUT_CLIP_LEVEL ) \
		 status |= OUT1_CLIP; \
	  if( fabs(out2) > DSP_FLOAT_OUTPUT_CLIP_LEVEL ) \
		 status |= OUT2_CLIP; \
}
#endif

/* END OF COMMON I/O MACROS */
