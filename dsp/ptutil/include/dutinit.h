/* (C) COPYRIGHT 1994-1997 Power Technology. All Rights Reserved.
 * This document and file contain highly confidential information
 * which is not to be disclosed or in any other way retransmitted
 * without the express written consent of Power Technology.
 */
/*
 * FILE: dutinit.h
 * DATE: 7/7/96
 * AUTOHOR: Paul F. Titchener
 * DESCRIPTION: Macros to run A/D and D/A converters, AES
 * This version had mods to run AES transmitter from receiver clock
 */

static long main_mem_size = 0;	/* Sizes of primary, secondary mem banks */
static long exp_mem_size = 0L;
static long dma_buf_size = 16000L;	/* Length of dma_buffers in longs */
static long dma_read_size = 16000L;	/* Length of dma_buffers in longs */

/* Defines, Macros and globals for DSPFX1 Board */
#if (defined(DSPFX1) & defined(DSP_TARGET)) 
/* AES oriented defines */
#define AES_STAT1_MASK 0x08 /* ERF */ 
#define AES_STAT2_MASK 0x1E /* CONF, LOCK, CODE, PARITY */ 

#define RED_LED 0x10
#define GREEN_LED 0x20
 
#define AD_DATA_REG (SER_0_ADR + 12)
#define AD_CNTRL_REG SER_0_ADR
#define DA_DATA_REG (SER_0_ADR + 8)
#define DA_CNTRL_REG SER_0_ADR
#define DA_MUTE_VAL 0x00040000L; 	/* mute output value	*/

unsigned codec_control(unsigned cdin);

struct io_reg {
	volatile unsigned comd_stat;            /* command 0 and status reg */
	volatile unsigned comd1;                /* command 1 */
	volatile unsigned codec_clk;            /* codec clock */
	volatile unsigned xfr_reg;              /* transfer registers   */
};

struct ser_reg {
	volatile unsigned global_ctl;
	volatile unsigned res0;
	volatile unsigned xmit_ctl;
	volatile unsigned rcv_ctl;
	volatile unsigned timer_ctl;
	volatile unsigned timer_cnt;
	volatile unsigned timer_per;
	volatile unsigned res1;
	volatile unsigned xmit_data;
	volatile unsigned res2;
	volatile unsigned res3;
	volatile unsigned res4;
	volatile unsigned rcv_data;
};

struct aes_reg {
	volatile unsigned x_stat0;		/* status reg 0		 */
	volatile unsigned x_ctl1;		/* control reg 1	 */
	volatile unsigned x_ctl2;		/* control reg 2	 */
	volatile unsigned x_ctl3;		/* control reg 3	 */
	volatile unsigned x_udat[4];	/* user data		 */
	volatile unsigned x_csdat[24];	/* C. S. data		 */
	volatile unsigned r_st1_ie1;	/* status 1 IE reg 1 */
	volatile unsigned r_st2_ie2;	/* status 2 IE reg 2 */
	volatile unsigned r_ctl1;		/* control reg 1	 */
	volatile unsigned r_ctl2;		/* control reg 2	 */
	volatile unsigned r_udat[4];	/* user data		 */
	volatile unsigned r_csdat[24];	/* C. S. data		 */
	volatile unsigned cmd_stat0;	/* command & status reg 0 */
	volatile unsigned reserved[31];	/* nothing here		 */
	volatile unsigned cmd_stat1;	/* command & status reg 1 */
};

/* A/D, D/A and initialization functions for DSPFX1 */
struct io_reg *io_adr = (struct io_reg *)(C32_IO_ADR);
struct ser_reg *ser_adr = (struct ser_reg *)(SER_0_ADR);
struct aes_reg *aes_adr = (struct aes_reg *)(0x810000);

unsigned command0_shadow;               /* shadow register for command 0 reg */
unsigned command1_shadow;               /* shadow register for command 1 reg */
unsigned aes_x_ctl2_shadow;             /* shadow register for AES xmitter ctl2  */
unsigned rev_c_board;                   /* For specifying board rev */

/* For counting number of entries into AES interrupt routine */
long aes_int_count = 0;
										/* Shadows are for examining values later */
void T3x_hinit(void);
void delay_us(unsigned t);              /* delay in us  */
void putchr(long d);                    /* long getchr(void); */

unsigned codec_control(unsigned cdin)
{
	int i;
	unsigned cdout;

    if( rev_c_board ) /* Different reset mechanism (shares bits) */
	  io_adr->comd1 = command1_shadow = command1_shadow | 0x85;	/* release reset 0x85 = 10000101 */
    else
	  io_adr->comd1 = command1_shadow = command1_shadow | 0x06; /* release reset 0x06 = 00000110 */	
	
	io_adr->comd1 = command1_shadow = command1_shadow & ~0x04;  /* drop CCS */
	cdin &= 0x7FFFFF00L;
	for (i = 0; i < 32; i++) {
		io_adr->codec_clk = cdin;
		cdin <<= 1;
		cdout = cdout << 1 | io_adr->codec_clk & 1;
	}
	/* Need code change here too ? */
	/* io_adr->comd1 = command1_shadow = command1_shadow | 0x06; */ /* lift CCS REV B version */
	io_adr->comd1 = command1_shadow = command1_shadow | 0x04;       /* lift CCS REV C version */
	return cdout;
}

void T3x_hinit(void)    /* this will execute before main() */
/* If you use the small assembly code loader, it uses this */
{
	 /* Place interrupt vectors starting at 0x1000 */
	 void (**p)() = (void (**)())0x1000; /* pointer to array of functions   */
	 void c_int00();		/* program starting address		*/
	 void c_int01();		/* interrupt routine		*/
	 void c_int02();		/* interrupt routine		*/
	 void c_int03();		/* interrupt routine		*/
	 
	/* set up All Bus control registers and cache
	 * Turns on cache, makes interupt edge trigger
	 */
	*(unsigned *)(0x808064) = 0x4F0008;   /* 0 wait stb0, no bank */
	*(unsigned *)(0x808068) = 0x0F0008;   /* 0 wait stb1, no bank */
#ifndef DUIO_D
	*(unsigned *)(0x808060) = 0x000028;   /* For transfer, command, status AES reg. 0 - 1 wait iostb - 
										   * MODIFIED FROM TIGER32C - use for DSPFX without AES  */
#else										   
	*(unsigned *)(0x808060) = 0x000050;  /* From AES.C  1 - 2 wait iostb Added wait states for AES */
										 /* Seems to only add 4 cycles in BM mode (no input read) */
#endif
/* Sets one wait state for transfer, status, A/D regs */
/*	*(unsigned *)(0x808060) = 0x000048;*/   /* 2 wait iostb		*/

	asm("	OR	4800h,ST");    		/* enable cache memory + int on edge */
	/* asm("	OR	0800h,ST"); */   		/* enable cache memory + int on level */
	asm("	ANDN	2000h,ST");		/* disable global interrupts */ 

	asm("	LDI	0,IOF");       /* Sets up XF0 and XF1 to be inputs (data ready flags) */ 
	
#ifdef NILS_NEW_METHOD
	asm("	ANDN	0FFFDh,IF");	/* clear lower IF bits	*/
	asm("	OR  	0002h,IF"); 	/* flag CTHRF = 0 */				
#endif
	putchr(0xa5a5a5a5); /* For synchronizing with PC host */

	putchr(~0xa5a5a5a5); /* more syncing */
	
#ifdef INTON	
	/* p = (void (**)())c_int00; */	/* WAS BUGGED, NO LONGER NEEDED point to code start address */

	/* store interrupt address in next location */
	p[1] = c_int01;		/* CTHRF */	
	p[2] = c_int02;		/* ADINT/1(+EXTINT/) */		
	p[3] = c_int03;		/* PCIRQ */						

#ifndef NILS_NEW_METHOD
	asm("	ANDN	0FFFFh,IF");	/* clear lower IF bits	*/
#endif

	/* use 0002h for INT1, 0004h for INT2 etc. */
	/* asm("	OR  	0001h,IE"); */	/* enable INT0 bit- HOST TO C32 REGISTER FULL INTERRUPT */
	asm("	OR  	0004h,IE"); 	/* enable INT2 bit- AES and CODEC interupt bits */				

	asm("	OR	2000h,ST");	/* enable global interrupts */
#endif
}

#ifdef INTON
void c_int01()		/* interrupt routine; name is c_intxx (xx = 01..99)*/
{			/* used for HTC transfer register on INT0	*/
/*
	inchar = *xfer_reg;
	gotints++;
*/
	/* while(1); */
}

void c_int02()		/* interrupt routine; name is c_intxx (xx = 01..99)*/
{			/* used for HTC transfer register on INT0	*/
	/* while(1);	 */
}

void c_int03()		/* interrupt routine; name is c_intxx (xx = 01..99)*/
{
    /* Send error flag forever */
    {
	  	aes_adr->x_ctl2 = aes_x_ctl2_shadow = 0xA7;	/* 384x, V, release transmitter set RST, MUTE */
    	while(1)
    	{
			long status = AES_READ_ERROR;
			long transfer_state = STATUS_STATE;
	  		load_parameter(); /* If its been sent, loads a parameter into memory */
	  		if( *(volatile long *)(PC_TO_DSP_FLAGS) & PC_GOT_AES_READ_ERROR )
	  		{
				while(1);                                    
			}
				
	  		write_meters_and_status(0L, 0L, 0L, 0L, status, transfer_state);
	  	}
    }
}                
#endif /* INTON */

#ifdef NILS_NEW_METHOD
/* Nil's version that allows running with serial card */
void putchr(long d)
{
	do {
	    asm("	STI	IF,@_iflg"); \

	} while ((iflg & 0x02) == 0);

	asm("	ANDN	2,IF"); \

        io_adr->xfr_reg = d;

}
#else
/* Original version that checked status bit */
void putchr(long d)
{
	while (io_adr->comd_stat & 0x80)
		;
	io_adr->xfr_reg = d;

}
#endif /* NILS_NEW_METHOD */

void delay_us(unsigned t)  /* delay in us  */
{
	register struct ser_reg *sp;
	unsigned x;

	t = (t * 12) / 3;               /* count 3,333,333 = 1 sec */
	sp = ser_adr;

	while (--t)
		 x = sp->timer_cnt;                  /* create some delay    */
}
#endif /* defined(DSPFX1) & defined(DSP_TARGET) */

static void dutilGetMemSizes()
{
	/* Get memsize vals, then zero memory. 
	 * Place before app specific mem initialization.
	 * Also get DMA buffer size.
	 */
#ifdef DSP_TARGET
	wait_for_val(main_mem_size);
	wait_for_val(exp_mem_size);	
	wait_for_val(dma_buf_size);	
#endif
}

static void dutilInitMem()
{
#ifdef DSP_TARGET

	long i, len;
	float *tmp_ptr;
	tmp_ptr = (float *)(MEMBANK0_START);

	/* Zero DSP memory spaces */
	if( main_mem_size > (MEMBANK0_LEN) ) 
	   len = MEMBANK0_BIG_LEN;
	else
	   len = MEMBANK0_LEN;

	for(i=0; i<len; i++)
		tmp_ptr[i] = 0.0;
	tmp_ptr = (float *)(MEMBANK1_START);
	for(i=0; i<exp_mem_size; i++)
		tmp_ptr[i] = 0.0;

#endif
}

/*
 * MACRO: dutilSampFreqAIO()
 * DESCRIPTION: Set Analog A/D and D/A sampling rate
 */
#define dutilSampFreqAIO(i_sampling_freq) ;

static void dutilInitAIO()
{   
#ifdef DSP_TARGET
	register unsigned i, j, k;
	int c,d;
	long cdin;
	long dummy;
	long samp_freq_status, fx_link_flag, io_type, serial_num;
#endif
	
#if (defined(DUIO_B) | defined(DUIO_BA) | defined(DUIO_BD))
	/* Need access to these DMA global vars */
	extern long *in_data_buf0;
	extern long *in_data_buf1;
	extern long *out_data_buf0;
	extern long *out_data_buf1;
	extern long *read_in_buf;
	extern long *read_out_buf;
	extern long parm_address_MACRO;
	extern long address_valid_flag_MACRO;
#endif	

	/* Initialize common DSP parameter memory values */
	/* MOVED INTO INIT FUNCTION
	*(volatile int *)(PC_TO_DSP_FLAGS) = 0L;
	*/
	
	/* Get io flags and serial number- function com_SendIoInfo */
#ifdef DSP_TARGET
	wait_for_val(io_type);
	wait_for_val(fx_link_flag);
	wait_for_val(serial_num);
	
	if( serial_num >= 2101 ) /* Different clock settings for Rev C and above */
		rev_c_board = 1;
	else
		rev_c_board = 0;
#endif

#ifdef DUIO_D /* AES input and output case */             
#include "duio_d.h"
#endif

#ifdef DUIO_A /* ANALOG input and output case */
	wait_for_val(cdin);    /* Get sampling frequency from host */
	/* sampling frequency MF6 is 4, MF7 if bit 5 ,  MF8 is 6 */
	/* Bit 3 is oscillator, all zeros is 44.1khz */
	/* cdin = 0; *//* MF6,7,8 and OSCSEL all zero for 44.1khz */
	/* cdin |= 72; */ /* Set MF8 and OSCSEL for 32khz */
	/* cdin |= 8 */ /* Set OSCSEL (only) for 48khz */
	*(volatile int *)(DSP_SAMPLING_FREQ) = cdin; /* Set value in memory */

#include "duio_a.h"

#endif

#ifdef DUIO_B

#include "duio_b.h" /* DMA input and output case */

	wait_for_val(cdin);    /* Get sampling frequency from host */
	*(volatile int *)(DSP_SAMPLING_FREQ) = cdin; /* Set value in memory */

#endif

#ifdef DUIO_BA

#include "duio_b.h" /* DMA input and output case */

	wait_for_val(cdin);    /* Get sampling frequency from host */
	*(volatile int *)(DSP_SAMPLING_FREQ) = cdin; /* Set value in memory */

#include "duio_a.h"

#endif
}
