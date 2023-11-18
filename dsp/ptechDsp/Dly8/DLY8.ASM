******************************************************
*    TMS320C30 C COMPILER     Version 4.70
******************************************************
;	C:\DSPTOOLS\AC30.EXE -l -x -v32 -DDUIO_BA -D -D -DDSP_SAMP_FREQ=44100.0 -DROOM_SIZE=1.00 dly8.c C:\WINDOWS\TEMP\dly8.if 
;	C:\DSPTOOLS\OPT30.EXE -p0 -m -v32 -s -O3 C:\WINDOWS\TEMP\dly8.if C:\WINDOWS\TEMP\dly8.opt 
;	C:\DSPTOOLS\CG30.EXE -n -c -m -v32 -n -a C:\WINDOWS\TEMP\dly8.opt dly8.asm C:\WINDOWS\TEMP\dly8.tmp 
	.version	32
FP	.set		AR3
	.globl	_asin
	.globl	_acos
	.globl	_atan
	.globl	_atan2
	.globl	_cos
	.globl	_cosh
	.globl	_exp
	.globl	_fabs
	.globl	_frexp
	.globl	_ldexp
	.globl	_log
	.globl	_log10
	.globl	_modf
	.globl	_pow
	.globl	_sin
	.globl	_sinh
	.globl	_sqrt
	.globl	_tan
	.globl	_tanh
	.globl	_ceil
	.globl	_floor
	.globl	_fmod
	.globl	_iflg

	.sect	".cinit"
	.word	1,_main_mem_size
	.word	131072
	.bss	_main_mem_size,1

	.word	1,_exp_mem_size
	.word	0
	.bss	_exp_mem_size,1

	.word	1,_dma_buf_size
	.word	16000
	.bss	_dma_buf_size,1

	.word	1,_dma_read_size
	.word	16000
	.bss	_dma_read_size,1
	.globl	_codec_control

	.word	IS1,_io_adr
	.word	8585200
IS1	.set	1
	.globl	_io_adr
	.bss	_io_adr,1

	.word	IS2,_ser_adr
	.word	8421440
IS2	.set	1
	.globl	_ser_adr
	.bss	_ser_adr,1

	.word	IS3,_aes_adr
	.word	8454144
IS3	.set	1
	.globl	_aes_adr
	.bss	_aes_adr,1
	.globl	_command0_shadow
	.globl	_command1_shadow
	.globl	_aes_x_ctl2_shadow
	.globl	_rev_c_board

	.word	1,_aes_int_count
	.word	0
	.globl	_aes_int_count
	.bss	_aes_int_count,1
	.globl	_T3x_hinit
	.globl	_delay_us
	.globl	_putchr
	.globl	_codec_control
	.globl	_T3x_hinit
	.globl	_putchr
	.globl	_delay_us
	.text

	.sect	".cinit"
	.word	1,_out_meter1_dma
	.word	0
	.bss	_out_meter1_dma,1

	.word	1,_out_meter2_dma
	.word	0
	.bss	_out_meter2_dma,1

	.word	1,_comd_stat
	.word	8585200
	.globl	_comd_stat
	.bss	_comd_stat,1

	.word	1,_xfer_reg
	.word	8585203
	.globl	_xfer_reg
	.bss	_xfer_reg,1
	.globl	_in_data_buf0
	.globl	_in_data_buf1
	.globl	_out_data_buf0
	.globl	_out_data_buf1
	.globl	_read_in_buf
	.globl	_read_out_buf

	.word	1,_address_valid_flag_MACRO
	.word	0
	.bss	_address_valid_flag_MACRO,1
	.globl	_main
	.globl	_read_in_buf
	.bss	_read_in_buf,1
	.globl	_read_out_buf
	.bss	_read_out_buf,1
	.globl	_command0_shadow
	.bss	_command0_shadow,1
	.globl	_out_data_buf0
	.bss	_out_data_buf0,1
	.globl	_out_data_buf1
	.bss	_out_data_buf1,1
	.globl	_aes_x_ctl2_shadow
	.bss	_aes_x_ctl2_shadow,1
	.globl	_command1_shadow
	.bss	_command1_shadow,1
	.bss	_parm_address_MACRO,1
	.globl	_rev_c_board
	.bss	_rev_c_board,1
	.globl	_in_data_buf0
	.bss	_in_data_buf0,1
	.globl	_in_data_buf1
	.bss	_in_data_buf1,1
	.globl	_iflg
	.bss	_iflg,1
	.text
	.globl	_putchr
******************************************************
* FUNCTION DEF : _putchr
******************************************************
_putchr:
	PUSH	FP
	LDI	SP,FP
L2:
***	-----------------------g2:
*** 218	-----------------------    asm("\tSTI\tIF,@_iflg");
	STI	IF,@_iflg
*** 219	-----------------------    if ( !(iflg&2) ) goto g2;
	LDI	@_iflg,R0
	TSTB	02h,R0
	BZ	L2
*** 222	-----------------------    asm("\tANDN\t2,IF");
	ANDN	2,IF
*** 223	-----------------------    io_adr->xfr_reg = d;
	LDI	@_io_adr,AR0
	LDI	*-FP(2),R0
	STI	R0,*+AR0(3)
***  	-----------------------    return;
EPI0_1:
	LDI	*-FP(1),R1
	BD	R1
	LDI	*FP,FP
	NOP
	SUBI	2,SP
***	B	R1	;BRANCH OCCURS
******************************************************
* FUNCTION DEF : _dutilGetMemSizes
******************************************************
_dutilGetMemSizes:
*
* AR2	assigned to temp var  U$0
*
***  	-----------------------    U$0 = io_adr;
	LDI	@_io_adr,AR2
L4:
***	-----------------------g2:
*** 257	-----------------------    if ( U$0->comd_stat&0x40 ) goto g2;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L4
*** 257	-----------------------    main_mem_size = U$0->xfr_reg;
	LDI	*+AR2(3),R1
	STI	R1,@_main_mem_size
L7:
***	-----------------------g5:
*** 258	-----------------------    if ( U$0->comd_stat&0x40 ) goto g5;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L7
*** 258	-----------------------    exp_mem_size = U$0->xfr_reg;
	LDI	*+AR2(3),R1
	STI	R1,@_exp_mem_size
L10:
***	-----------------------g8:
*** 259	-----------------------    if ( U$0->comd_stat&0x40 ) goto g8;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L10
*** 259	-----------------------    dma_buf_size = U$0->xfr_reg;
	LDI	*+AR2(3),R1
	STI	R1,@_dma_buf_size
***  	-----------------------    return;
EPI0_2:
	RETS
******************************************************
* FUNCTION DEF : _dutilInitMem
******************************************************
_dutilInitMem:
*
* R2	assigned to temp var  K$14
* R3	assigned to temp var  C$3
* AR2	assigned to temp var  U$15
* AR2	assigned to variable  tmp_ptr
* RC	assigned to temp var  L$1
* RC	assigned to temp var  L$2
* RC	assigned to variable  len
*
*** 270	-----------------------    len = (main_mem_size > 28672) ? 126976 : 28672;
	LDI	@_main_mem_size,R0
	CMPI	28672,R0
	LDIGT	@CONST+0,RC
	LDILE	28672,RC
***  	-----------------------    tmp_ptr = (float*)0x2000;
	LDI	8192,AR2
***  	-----------------------    U$15 = tmp_ptr;
***  	-----------------------    K$14 = 0.0F;
	LDF	0.0,R2
***  	-----------------------    L$1 = len-1;
	SUBI	1,RC
***	-----------------------g2:
***	-----------------------g9:
	RPTS
*** 275	-----------------------    *U$15++ = K$14;
	STF	R2,*AR2++
*** 274	-----------------------    if ( --L$1 >= 0 ) goto g9;
*** 278	-----------------------    if ( (C$3 = exp_mem_size) <= 0 ) goto g7;
	LDI	@_exp_mem_size,R3
	RETSLE
*** 277	-----------------------    tmp_ptr = (float*)0x900000;
	LDI	@CONST+1,AR2
***  	-----------------------    U$15 = tmp_ptr;
***  	-----------------------    K$14 = 0.0F;
	LDF	0.0,R2
***  	-----------------------    L$2 = C$3-1;
	LDI	R3,RC
	SUBI	1,RC
***	-----------------------g6:
***	-----------------------g8:
	RPTS
*** 279	-----------------------    *U$15++ = K$14;
	STF	R2,*AR2++
*** 278	-----------------------    if ( --L$2 >= 0 ) goto g8;
***	-----------------------g7:
***  	-----------------------    return;
EPI0_3:
	RETS
	.globl	_delay_us
******************************************************
* FUNCTION DEF : _delay_us
******************************************************
_delay_us:
	PUSH	FP
	LDI	SP,FP
*
* R2	assigned to parameter t
* AR2	assigned to variable  sp
*
	LDI	*-FP(2),R2
*** 242	-----------------------    t = t*12u/3u;
	LDI	12,R0
	MPYI	R2,R0
	CMPI	@CONST+2,R0
	BHS	LL6
	BD	LL7
	ADDI	1,R0
	MPYI	21845,R0
	LSH	-16,R0
***	B	LL7	;BRANCH OCCURS
LL6:
	LDI	3,R1
	CALL	DIV_U30
LL7:
	BD	L21
	LDI	R0,R2
*** 243	-----------------------    sp = &ser_adr[5];
	LDI	@_ser_adr,AR2
	ADDI	5,AR2
*** 245	-----------------------    goto g2;
***	B	L21	;BRANCH OCCURS
L20:
***	-----------------------g1:
*** 246	-----------------------    *(unsigned*)sp;
	NOP	*AR2
L21:
***	-----------------------g2:
*** 245	-----------------------    if ( --t ) goto g1;
	SUBI	1,R2
	BNZ	L20
***  	-----------------------    return;
EPI0_4:
	LDI	*-FP(1),R1
	BD	R1
	LDI	*FP,FP
	NOP
	SUBI	2,SP
***	B	R1	;BRANCH OCCURS
	.globl	_codec_control
******************************************************
* FUNCTION DEF : _codec_control
******************************************************
_codec_control:
	PUSH	FP
	LDI	SP,FP
	PUSH	AR4
*
* R2	assigned to variable  cdout
* R3	assigned to parameter cdin
* AR2	assigned to temp var  U$7
* AR4	assigned to temp var  C$2
* AR4	assigned to temp var  C$3
* RC	assigned to temp var  L$1
*
	LDI	*-FP(2),R3
*** 99	-----------------------    if ( rev_c_board ) goto g2;
	LDI	@_rev_c_board,R0
	BNZ	L23
*** 102	-----------------------    C$3 = io_adr;
	LDI	@_io_adr,AR4
*** 102	-----------------------    U$7 = &C$3[2];
	LDI	AR4,AR2
	ADDI	2,AR2
*** 102	-----------------------    C$3->comd1 = command1_shadow |= 6u;
	LDI	@_command1_shadow,R1
	BD	L24
	OR	06h,R1
	STI	R1,@_command1_shadow
	STI	R1,*+AR4(1)
*** 102	-----------------------    goto g3;
***	B	L24	;BRANCH OCCURS
L23:
***	-----------------------g2:
*** 100	-----------------------    C$2 = io_adr;
	LDI	@_io_adr,AR4
*** 100	-----------------------    U$7 = &C$2[2];
	LDI	AR4,AR2
	ADDI	2,AR2
*** 100	-----------------------    C$2->comd1 = command1_shadow |= 0x85u;
	LDI	@_command1_shadow,R1
	OR	085h,R1
	STI	R1,@_command1_shadow
	STI	R1,*+AR4(1)
L24:
***	-----------------------g3:
*** 104	-----------------------    *((unsigned*)U$7-1) = command1_shadow &= 0xfffffffbu;
	LDI	@_command1_shadow,R0
	ANDN	04h,R0
	STI	R0,@_command1_shadow
	STI	R0,*-AR2(1)
*** 105	-----------------------    cdin &= 0x7fffff00u;
	AND	@CONST+3,R3
***  	-----------------------    L$1 = 31;
	LDI	31,RC
***	-----------------------g5:
***	-----------------------g7:
	RPTB	L28
*** 107	-----------------------    *(unsigned*)U$7 = cdin;
	STI	R3,*AR2
*** 108	-----------------------    cdin <<= 1;
	LSH	1,R3
*** 109	-----------------------    cdout = cdout<<1|*(unsigned*)U$7&1u;
	LSH	1,R2
	LDI	1,R0
	AND	R0,*AR2,R1
L28:	OR	R1,R2
*** 106	-----------------------    if ( --L$1 >= 0 ) goto g7;
*** 113	-----------------------    *((unsigned*)U$7-1) = command1_shadow |= 4u;
	LDI	@_command1_shadow,R1
	OR	04h,R1
	STI	R1,@_command1_shadow
	STI	R1,*-AR2(1)
*** 114	-----------------------    return cdout;
	LDI	R2,R0
EPI0_5:
	LDI	*-FP(1),R1
	BD	R1
	LDI	*FP,FP
	POP	AR4
	SUBI	2,SP
***	B	R1	;BRANCH OCCURS
******************************************************
* FUNCTION DEF : _dutilInitAIO
******************************************************
_dutilInitAIO:
	PUSH	FP
	LDI	SP,FP
	ADDI	1,SP
	PUSH	R4
	PUSH	R5
	PUSH	AR4
	.globl	_in_data_buf0
	.globl	_in_data_buf1
	.globl	_out_data_buf0
	.globl	_out_data_buf1
	.globl	_read_in_buf
	.globl	_read_out_buf
	.globl	_parm_address_MACRO
	.globl	_address_valid_flag_MACRO
*
* R2	assigned to temp var  U$17
* R2	assigned to variable  parm_MACRO
* R3	assigned to temp var  C$5
* R3	assigned to temp var  U$22
* R4	assigned to temp var  U$45
* R4	assigned to variable  cdin
* R5	assigned to temp var  C$4
* R5	assigned to temp var  K$41
* AR2	assigned to temp var  L$1
* AR2	assigned to temp var  U$4
* AR4	assigned to temp var  C$2
* AR4	assigned to temp var  C$3
* BK	assigned to temp var  U$11
* RC	assigned to variable  fx_link_flag
* RS	assigned to temp var  C$6
*
*** 313	-----------------------    *(int*)0x87fe00 = 0;
	LDI	@CONST+4,AR0
	LDI	0,R0
	STI	R0,*AR0
***  	-----------------------    U$4 = io_adr;
	LDI	@_io_adr,AR2
L30:
***	-----------------------g2:
*** 317	-----------------------    if ( U$4->comd_stat&0x40 ) goto g2;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L30
*** 317	-----------------------    U$4->xfr_reg;
	NOP	*+AR2(3)
L33:
***	-----------------------g5:
*** 318	-----------------------    if ( U$4->comd_stat&0x40 ) goto g5;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L33
*** 318	-----------------------    fx_link_flag = U$4->xfr_reg;
	LDI	*+AR2(3),RC
L36:
***	-----------------------g8:
*** 319	-----------------------    if ( U$4->comd_stat&0x40 ) goto g8;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L36
*** 319	-----------------------    if ( (serial_num = U$4->xfr_reg) >= 2101 ) goto g11;
	LDI	*+AR2(3),R1
	STI	R1,*+FP(1)
	CMPI	2101,R1
	BGE	L39
*** 324	-----------------------    U$11 = rev_c_board = 0u;
	BD	L40
	LDI	0,BK
	NOP
	STI	BK,@_rev_c_board
*** 324	-----------------------    goto g12;
***	B	L40	;BRANCH OCCURS
L39:
***	-----------------------g11:
*** 322	-----------------------    rev_c_board = U$11 = 1u;
	LDI	1,BK
	STI	BK,@_rev_c_board
L40:
***	-----------------------g12:
*** 13	-----------------------    if ( exp_mem_size > 0 ) goto g14;
	LDI	@_exp_mem_size,R2
	BGT	L42
*** 16	-----------------------    U$17 = dma_buf_size;
	LDI	@_dma_buf_size,R2
*** 16	-----------------------    in_data_buf0 = U$22 = (int*)(135168-U$17*4);
	LDI	2,R1
	BD	L43
	LSH	R1,R2,R3
	SUBRI	@CONST+5,R3
	STI	R3,@_in_data_buf0
*** 16	-----------------------    goto g15;
***	B	L43	;BRANCH OCCURS
L42:
***	-----------------------g14:
*** 14	-----------------------    U$17 = dma_buf_size;
	LDI	@_dma_buf_size,R2
*** 14	-----------------------    in_data_buf0 = U$22 = (int*)(9568256-U$17*4);
	LDI	2,R1
	LSH	R1,R2,R3
	SUBRI	@CONST+6,R3
	STI	R3,@_in_data_buf0
L43:
***	-----------------------g15:
*** 18	-----------------------    in_data_buf1 = C$6 = &U$22[U$17];
	ADDI	R2,R3,RS
	STI	RS,@_in_data_buf1
*** 19	-----------------------    out_data_buf0 = &U$22[2*U$17];
	LDI	1,R1
	LSH	R1,R2,R1
	ADDI	R3,R1
	STI	R1,@_out_data_buf0
*** 20	-----------------------    out_data_buf1 = C$5 = &U$22[3*U$17];
	MPYI	3,R2
	ADDI	R2,R3
	STI	R3,@_out_data_buf1
*** 21	-----------------------    read_in_buf = C$6;
	STI	RS,@_read_in_buf
*** 22	-----------------------    read_out_buf = C$5;
	STI	R3,@_read_out_buf
L45:
***	-----------------------g17:
*** 357	-----------------------    if ( U$4->comd_stat&0x40 ) goto g17;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L45
*** 357	-----------------------    cdin = U$4->xfr_reg;
	LDI	*+AR2(3),R4
*** 358	-----------------------    *(int*)0x87fe03 = cdin;
	LDI	@CONST+7,AR0
	STI	R4,*AR0
*** 17	-----------------------    C$3 = aes_adr;
	LDI	@_aes_adr,AR4
*** 17	-----------------------    C$3->cmd_stat0 = C$4 = 0u;
	LDI	0,R5
	STI	R5,*+AR4(64)
*** 18	-----------------------    C$3->cmd_stat1 = K$41 = C$4;
	STI	R5,*+AR4(96)
*** 24	-----------------------    if ( U$11 ) goto g20;
	CMPI	0,BK
	BNZ	L48
*** 39	-----------------------    U$4->comd1 = command1_shadow = (unsigned)cdin&0xffffff78u|0x85u;
	LDI	135,R1
	ANDN	R1,R4,R2
	BD	L51
	OR	085h,R2
	STI	R2,@_command1_shadow
	STI	R2,*+AR2(1)
*** 39	-----------------------    goto g23;
***	B	L51	;BRANCH OCCURS
L48:
***	-----------------------g20:
*** 27	-----------------------    if ( fx_link_flag ) goto g22;
	CMPI	0,RC
	BNZ	L50
*** 34	-----------------------    U$4->comd_stat = command0_shadow = 32u;
	LDI	32,R1
	STI	R1,@_command0_shadow
	STI	R1,*AR2
*** 35	-----------------------    U$4->comd1 = command1_shadow = (unsigned)cdin&0xffffff78u|4u;
	LDI	135,R2
	ANDN	R2,R4,R3
	BD	L51
	OR	04h,R3
	STI	R3,@_command1_shadow
	STI	R3,*+AR2(1)
*** 35	-----------------------    goto g23;
***	B	L51	;BRANCH OCCURS
L50:
***	-----------------------g22:
*** 29	-----------------------    U$4->comd_stat = command0_shadow = 80u;
	LDI	80,R1
	STI	R1,@_command0_shadow
	STI	R1,*AR2
*** 30	-----------------------    U$4->comd1 = command1_shadow = (unsigned)cdin&0xffffff78u|6u;
	LDI	135,R2
	ANDN	R2,R4,R3
	OR	06h,R3
	STI	R3,@_command1_shadow
	STI	R3,*+AR2(1)
L51:
***	-----------------------g23:
*** 41	-----------------------    C$2 = ser_adr;
	LDI	@_ser_adr,AR4
*** 41	-----------------------    C$2->global_ctl = 3932160u;
	LDI	@CONST+8,R1
	STI	R1,*AR4
*** 42	-----------------------    C$2->xmit_ctl = C$2->rcv_ctl = 273u;
	LDI	273,R2
	STI	R2,*+AR4(3)
	STI	R2,*+AR4(2)
*** 43	-----------------------    delay_us(50000u);
	LDI	@CONST+9,R3
	PUSH	R3
	CALL	_delay_us
	SUBI	1,SP
*** 44	-----------------------    C$2->global_ctl = 205258752u;
	LDI	@CONST+10,R0
	STI	R0,*AR4
*** 45	-----------------------    C$2->xmit_data = K$41;
	STI	R5,*+AR4(8)
*** 49	-----------------------    codec_control((unsigned)(cdin |= 0x40000));
	OR	@CONST+11,R4
	PUSH	R4
	CALL	_codec_control
	SUBI	1,SP
*** 51	-----------------------    codec_control(U$45 = cdin);
	PUSH	R4
	CALL	_codec_control
	SUBI	1,SP
L53:
***	-----------------------g25:
*** 52	-----------------------    if ( (codec_control(U$45)&0x400004) != 0x400004 ) goto g25;
	PUSH	R4
	CALL	_codec_control
	SUBI	1,SP
	AND	@CONST+12,R0
	CMPI	@CONST+12,R0
	BNZ	L53
***  	-----------------------    L$1 = 9999;
	LDI	9999,AR2
***	-----------------------g28:
***	-----------------------g38:
L66:
*** 61	-----------------------    if ( *(int*)0x87fe0d ) goto g34;
	LDI	@CONST+13,AR0
	LDI	*AR0,R0
	BNZ	L63
*** 61	-----------------------    if ( address_valid_flag_MACRO ) goto g32;
	LDI	@_address_valid_flag_MACRO,R0
	BNZ	L60
*** 61	-----------------------    asm("\tSTI\tIOF,@_iflg");
	STI	IOF,@_iflg
*** 61	-----------------------    if ( iflg&0x8 ) goto g34;
	LDI	@_iflg,R1
	TSTB	08h,R1
	BNZ	L63
*** 61	-----------------------    parm_address_MACRO = io_adr->xfr_reg;
	LDI	@_io_adr,AR1
	LDI	*+AR1(3),R1
	BD	L63
	STI	R1,@_parm_address_MACRO
*** 61	-----------------------    address_valid_flag_MACRO = 1;
	LDI	1,R0
	STI	R0,@_address_valid_flag_MACRO
*** 61	-----------------------    goto g34;
***	B	L63	;BRANCH OCCURS
L60:
***	-----------------------g32:
*** 61	-----------------------    asm("\tSTI\tIOF,@_iflg");
	STI	IOF,@_iflg
*** 61	-----------------------    if ( iflg&0x8 ) goto g34;
	LDI	@_iflg,R1
	TSTB	08h,R1
	BNZ	L63
*** 61	-----------------------    parm_MACRO = io_adr->xfr_reg;
	LDI	@_io_adr,AR1
	LDI	*+AR1(3),R2
*** 61	-----------------------    *(int*)parm_address_MACRO = parm_MACRO;
	LDI	@_parm_address_MACRO,AR1
	STI	R2,*AR1
*** 61	-----------------------    address_valid_flag_MACRO = 0;
	LDI	0,R0
	STI	R0,@_address_valid_flag_MACRO
L63:
***	-----------------------g34:
***	-----------------------g35:
*** 62	-----------------------    if ( !(*(unsigned*)0x808040&1) ) goto g35;
	LDI	@CONST+14,AR0
	LDI	1,R0
	TSTB	R0,*AR0
	BZ	L63
*** 63	-----------------------    *(unsigned*)0x80804c;
	DBD	AR2,L66
	LDI	@CONST+15,AR1
	NOP
	NOP	*AR1
*** 57	-----------------------    if ( --L$1 >= 0 ) goto g38;
***	DB	AR2,L66	;BRANCH OCCURS
*** 66	-----------------------    *(unsigned*)0x808048 = K$41;
	LDI	@CONST+16,AR2
	STI	R5,*AR2
*** 67	-----------------------    *(unsigned*)0x808048 = K$41;
	STI	R5,*AR2
*** 69	-----------------------    codec_control((unsigned)(cdin &= 0xfffbffff));
	AND	@CONST+17,R4
	PUSH	R4
	CALL	_codec_control
	SUBI	1,SP
***  	-----------------------    return;
EPI0_6:
	LDI	*-FP(1),R1
	LDI	*FP,FP
	POP	AR4
	BD	R1
	POP	R5
	POP	R4
	SUBI	3,SP
***	B	R1	;BRANCH OCCURS
	.globl	_main
******************************************************
* FUNCTION DEF : _main
******************************************************
_main:
	PUSH	FP
	LDI	SP,FP
	ADDI	43,SP
	PUSH	R4
	PUSH	R5
	PUSHF	R6
	PUSHF	R7
	PUSH	AR4
	PUSH	AR5
	PUSH	AR6
	PUSH	AR7

	.sect	".cinit"
	.word	1,STATIC_1
	.word	0
	.bss	STATIC_1,1

	.word	1,STATIC_2
	.word	0
	.bss	STATIC_2,1
	.text
*
* R2	assigned to temp var  C$10
* R2	assigned to temp var  C$11
* R2	assigned to temp var  U$94
* R2	assigned to variable  line_len
* R3	assigned to temp var  C$7
* R3	assigned to temp var  C$8
* R3	assigned to temp var  C$9
* R3	assigned to temp var  L$1
* R3	assigned to temp var  Y$0
* R3	assigned to temp var  Y$1
* R3	assigned to temp var  Y$2
* R3	assigned to temp var  Y$3
* R3	assigned to temp var  Y$4
* R3	assigned to temp var  Y$5
* R3	assigned to temp var  Y$6
* R3	assigned to temp var  Y$7
* R3	assigned to variable  i_val2_MACRO
* R3	assigned to variable  in_meter1
* R3	assigned to variable  in_meter2
* R3	assigned to variable  j
* R3	assigned to variable  mute_val_MACRO
* R3	assigned to variable  out2
* R3	assigned to variable  parm_MACRO
* R3	assigned to variable  r_tmp_MACRO
* R4	assigned to variable  data_index
* R5	assigned to variable  delay2_start
* R5	assigned to variable  in2
* R6	assigned to variable  delay7_start
* R6	assigned to variable  in1
* R7	assigned to variable  delay6_start
* R7	assigned to variable  out1
* AR2	assigned to temp var  U$123
* AR2	assigned to temp var  U$173
* AR2	assigned to temp var  U$181
* AR4	assigned to temp var  U$209
* AR4	assigned to temp var  U$221
* AR4	assigned to temp var  U$321
* AR4	assigned to temp var  U$330
* AR4	assigned to variable  delay5_start
* AR4	assigned to variable  pan_left_gain
* AR4	assigned to variable  pan_right_gain
* AR4	assigned to variable  rp_MACRO
* AR4	assigned to variable  tmp_ptr
* AR5	assigned to variable  ptr0
* AR6	assigned to temp var  U$318
* AR6	assigned to temp var  U$328
* AR6	assigned to variable  delay
* AR6	assigned to variable  delay4_start
* AR6	assigned to variable  element_gain
* AR6	assigned to variable  rp_MACRO
* AR7	assigned to temp var  U$229
* AR7	assigned to temp var  U$319
* AR7	assigned to variable  delay3_start
* IR1	assigned to variable  dma_mode_flag
* BK	assigned to temp var  U$101
* BK	assigned to temp var  U$116
* RC	assigned to temp var  C$6
* RC	assigned to temp var  K$121
* RC	assigned to temp var  L$2
* RC	assigned to temp var  L$3
* RC	assigned to temp var  L$4
* RC	assigned to temp var  L$5
* RC	assigned to variable  i_val1_MACRO
* RC	assigned to variable  in_meter1
*
*** 46	-----------------------    dutilGetMemSizes();
	CALL	_dutilGetMemSizes
*** 47	-----------------------    dutilInitMem();
	CALL	_dutilInitMem
*** 52	-----------------------    *(float*)0x87fe05 = C$11 = 0.0F;
	LDF	0.0,R2
	LDI	@CONST+18,AR0
	STF	R2,*AR0
*** 53	-----------------------    *(float*)0x87fe0b = C$11;
	LDI	@CONST+19,AR1
	STF	R2,*AR1
*** 54	-----------------------    *(float*)0x87fe0a = C$11;
	LDI	@CONST+20,AR2
	STF	R2,*AR2
*** 56	-----------------------    *(float*)0x87fe24 = C$11;
	LDI	@CONST+21,AR2
	STF	R2,*AR2
*** 57	-----------------------    *(float*)0x87fe54 = C$11;
	LDI	@CONST+22,AR2
	STF	R2,*AR2
*** 58	-----------------------    *(float*)0x87fe34 = C$11;
	LDI	@CONST+23,AR2
	STF	R2,*AR2
*** 59	-----------------------    *(float*)0x87fe44 = C$11;
	LDI	@CONST+24,AR2
	STF	R2,*AR2
*** 60	-----------------------    *(float*)0x87fe25 = C$11;
	LDI	@CONST+25,AR2
	STF	R2,*AR2
*** 61	-----------------------    *(float*)0x87fe55 = C$11;
	LDI	@CONST+26,AR2
	STF	R2,*AR2
*** 62	-----------------------    *(float*)0x87fe35 = C$11;
	LDI	@CONST+27,AR2
	STF	R2,*AR2
*** 63	-----------------------    *(float*)0x87fe45 = C$11;
	LDI	@CONST+28,AR2
	STF	R2,*AR2
*** 64	-----------------------    *(float*)0x87fe26 = C$11;
	LDI	@CONST+29,AR2
	STF	R2,*AR2
*** 65	-----------------------    *(float*)0x87fe56 = C$11;
	LDI	@CONST+30,AR2
	STF	R2,*AR2
*** 66	-----------------------    *(float*)0x87fe36 = C$11;
	LDI	@CONST+31,AR2
	STF	R2,*AR2
*** 67	-----------------------    *(float*)0x87fe46 = C$11;
	LDI	@CONST+32,AR2
	STF	R2,*AR2
*** 68	-----------------------    *(float*)0x87fe27 = C$11;
	LDI	@CONST+33,AR2
	STF	R2,*AR2
*** 69	-----------------------    *(float*)0x87fe57 = C$11;
	LDI	@CONST+34,AR2
	STF	R2,*AR2
*** 70	-----------------------    *(float*)0x87fe37 = C$11;
	LDI	@CONST+35,AR2
	STF	R2,*AR2
*** 71	-----------------------    *(float*)0x87fe47 = C$11;
	LDI	@CONST+36,AR2
	STF	R2,*AR2
*** 72	-----------------------    *(float*)0x87fe28 = C$11;
	LDI	@CONST+37,AR2
	STF	R2,*AR2
*** 73	-----------------------    *(float*)0x87fe58 = C$11;
	LDI	@CONST+38,AR2
	STF	R2,*AR2
*** 74	-----------------------    *(float*)0x87fe38 = C$11;
	LDI	@CONST+39,AR2
	STF	R2,*AR2
*** 75	-----------------------    *(float*)0x87fe48 = C$11;
	LDI	@CONST+40,AR2
	STF	R2,*AR2
*** 76	-----------------------    *(float*)0x87fe29 = C$11;
	LDI	@CONST+41,AR2
	STF	R2,*AR2
*** 77	-----------------------    *(float*)0x87fe59 = C$11;
	LDI	@CONST+42,AR2
	STF	R2,*AR2
*** 78	-----------------------    *(float*)0x87fe39 = C$11;
	LDI	@CONST+43,AR2
	STF	R2,*AR2
*** 79	-----------------------    *(float*)0x87fe49 = C$11;
	LDI	@CONST+44,AR2
	STF	R2,*AR2
*** 80	-----------------------    *(float*)0x87fe2a = C$11;
	LDI	@CONST+45,AR2
	STF	R2,*AR2
*** 81	-----------------------    *(float*)0x87fe5a = C$11;
	LDI	@CONST+46,AR2
	STF	R2,*AR2
*** 82	-----------------------    *(float*)0x87fe3a = C$11;
	LDI	@CONST+47,AR2
	STF	R2,*AR2
*** 83	-----------------------    *(float*)0x87fe4a = C$11;
	LDI	@CONST+48,AR2
	STF	R2,*AR2
*** 84	-----------------------    *(float*)0x87fe2b = C$11;
	LDI	@CONST+49,AR2
	STF	R2,*AR2
*** 85	-----------------------    *(float*)0x87fe5b = C$11;
	LDI	@CONST+50,AR2
	STF	R2,*AR2
*** 86	-----------------------    *(float*)0x87fe3b = C$11;
	LDI	@CONST+51,AR2
	STF	R2,*AR2
*** 87	-----------------------    *(float*)0x87fe4b = C$11;
	LDI	@CONST+52,AR2
	STF	R2,*AR2
*** 90	-----------------------    *(int*)0x87fe14 = C$10 = 1;
	LDI	1,R2
	LDI	@CONST+53,AR2
	STI	R2,*AR2
*** 91	-----------------------    *(int*)0x87fe15 = C$10;
	LDI	@CONST+54,AR2
	STI	R2,*AR2
*** 92	-----------------------    *(int*)0x87fe16 = C$10;
	LDI	@CONST+55,AR2
	STI	R2,*AR2
*** 93	-----------------------    *(int*)0x87fe17 = C$10;
	LDI	@CONST+56,AR2
	STI	R2,*AR2
*** 94	-----------------------    *(int*)0x87fe18 = C$10;
	LDI	@CONST+57,AR2
	STI	R2,*AR2
*** 95	-----------------------    *(int*)0x87fe19 = C$10;
	LDI	@CONST+58,AR2
	STI	R2,*AR2
*** 96	-----------------------    *(int*)0x87fe1a = C$10;
	LDI	@CONST+59,AR2
	STI	R2,*AR2
*** 97	-----------------------    *(int*)0x87fe1b = C$10;
	LDI	@CONST+60,AR2
	STI	R2,*AR2
*** 101	-----------------------    dutilInitAIO();
	CALL	_dutilInitAIO
*** 106	-----------------------    U$94 = main_mem_size;
	LDI	@_main_mem_size,R2
*** 42	-----------------------    dma_mode_flag = 0u;
	LDI	0,IR1
*** 42	-----------------------    first_time_input = buf_num = 1u;
	LDI	1,R0
	STI	R0,*+FP(15)
	STI	R0,*+FP(14)
*** 106	-----------------------    C$9 = exp_mem_size;
	LDI	@_exp_mem_size,R3
*** 106	-----------------------    if ( U$94 > C$9 ) goto g4;
	CMPI	R3,R2
	BGT	L70
*** 114	-----------------------    if ( U$94 > 32768 ) goto g3;
	CMPI	@CONST+61,R2
	BGT	L69
*** 122	-----------------------    U$101 = dma_buf_size;
	LDI	@_dma_buf_size,BK
*** 122	-----------------------    line_len = (C$9-U$101*4)/8;
	LDI	2,R1
	LSH	R1,BK,R0
	SUBI	R0,R3,R0
	LDI	8,R1
	CALL	DIV_I30
	BD	L71
	LDI	R0,R2
	NOP
*** 123	-----------------------    ptr0 = (float*)0x900000;
	LDI	@CONST+1,AR5
*** 123	-----------------------    goto g5;
***	B	L71	;BRANCH OCCURS
L69:
***	-----------------------g3:
*** 116	-----------------------    line_len = U$94/8;
	LDI	R2,R0
	LDI	8,R1
	CALL	DIV_I30
	BD	L71
	LDI	R0,R2
***  	-----------------------    U$101 = dma_buf_size;
	LDI	@_dma_buf_size,BK
*** 117	-----------------------    ptr0 = (float*)0x2000;
	LDI	8192,AR5
***  	-----------------------    goto g5;
***	B	L71	;BRANCH OCCURS
L70:
***	-----------------------g4:
*** 108	-----------------------    U$101 = dma_buf_size;
	LDI	@_dma_buf_size,BK
*** 108	-----------------------    line_len = (U$94-U$101*4)/8;
	LDI	2,R1
	LSH	R1,BK,R0
	SUBI	R0,R2,R0
	LDI	8,R1
	CALL	DIV_I30
	LDI	R0,R2
*** 109	-----------------------    ptr0 = (float*)0x2000;
	LDI	8192,AR5
L71:
***	-----------------------g5:
*** 127	-----------------------    U$116 = U$101;
*** 127	-----------------------    if ( !(C$8 = (unsigned)U$101*4u) ) goto g9;
	LDI	2,R1
	LSH	R1,BK,R3
	BZ	L75
***  	-----------------------    U$123 = in_data_buf0;
	LDI	@_in_data_buf0,AR2
***  	-----------------------    K$121 = 0;
	LDI	0,RC
***  	-----------------------    L$1 = C$8;
L74:
***	-----------------------g8:
*** 127	-----------------------    *U$123++ = K$121;
	STI	RC,*AR2++
*** 127	-----------------------    if ( --L$1 ) goto g8;
	SUBI	1,R3
	BNZ	L74
L75:
***	-----------------------g9:
*** 127	-----------------------    data_index = U$101;
	LDI	BK,R4
*** 127	-----------------------    ((struct*)0x808000)->_gctrl._intval = C$6 = 0u;
	LDI	0,RC
	LDI	@CONST+62,AR0
	STI	RC,*AR0
*** 127	-----------------------    ((struct*)0x808010)->_gctrl._intval = C$6;
	LDI	@CONST+63,AR1
	STI	RC,*AR1
*** 127	-----------------------    ((struct*)0x808000)->transfer_counter = C$6;
	STI	RC,*+AR0(8)
*** 127	-----------------------    ((struct*)0x808010)->transfer_counter = C$6;
	STI	RC,*+AR1(8)
*** 127	-----------------------    ((struct*)0x808000)->source = C$7 = xfer_reg;
	LDI	@_xfer_reg,R3
	STI	R3,*+AR0(4)
*** 127	-----------------------    ((struct*)0x808000)->destination = U$122 = in_data_buf0;
	LDI	@_in_data_buf0,R0
	STI	R0,*+FP(3)
	STI	R0,*+AR0(6)
*** 127	-----------------------    ((struct*)0x808010)->source = out_data_buf0;
	LDI	@_out_data_buf0,R0
	STI	R0,*+AR1(4)
*** 127	-----------------------    ((struct*)0x808010)->destination = C$7;
	STI	R3,*+AR1(6)
*** 127	-----------------------    ((struct*)0x808000)->_gctrl._intval = 13632u;
	LDI	13632,R0
	STI	R0,*AR0
*** 127	-----------------------    ((struct*)0x808010)->_gctrl._intval = 13840u;
	LDI	13840,R1
	STI	R1,*AR1
*** 127	-----------------------    asm("\tPUSH AR2");
	PUSH AR2
*** 127	-----------------------    asm("\tLDI\t 2001h,AR2");
	LDI	 2001h,AR2
*** 127	-----------------------    asm("\tLSH  16,AR2");
	LSH  16,AR2
*** 127	-----------------------    asm("\tOR   AR2,IE");
	OR   AR2,IE
*** 127	-----------------------    asm("\tPOP  AR2");
	POP  AR2
*** 127	-----------------------    *(int*)0x87fe0d = K$121 = C$6;
	STI	RC,*+FP(5)
	LDI	@CONST+13,AR2
	STI	RC,*AR2
*** 142	-----------------------    delay0_start = ptr0;
	STI	AR5,*+FP(2)
*** 143	-----------------------    delay1_start = U$143 = &delay0_start[line_len];
	LDI	*+FP(2),R3
	ADDI	R2,R3
	STI	R3,*+FP(16)
	STI	R3,*+FP(1)
*** 144	-----------------------    delay2_start = &delay0_start[2*line_len];
	LDI	1,R3
	LSH	R3,R2,R5
	ADDI	*+FP(2),R5
*** 145	-----------------------    delay3_start = &delay0_start[3*line_len];
	LDI	3,R3
	MPYI	R3,R2,AR7
	ADDI	*+FP(2),AR7
*** 146	-----------------------    delay4_start = &delay0_start[4*line_len];
	LDI	2,R3
	LSH	R3,R2,AR6
	ADDI	*+FP(2),AR6
*** 147	-----------------------    delay5_start = &delay0_start[5*line_len];
	LDI	5,R3
	MPYI	R3,R2,AR4
	ADDI	*+FP(2),AR4
*** 148	-----------------------    delay6_start = &delay0_start[6*line_len];
	LDI	6,R3
	MPYI	R3,R2,R7
	ADDI	*+FP(2),R7
*** 149	-----------------------    delay7_start = &delay0_start[7*line_len];
	LDI	7,R3
	MPYI	R3,R2,R6
	ADDI	*+FP(2),R6
***  	-----------------------    U$173 = &old_delay[0];
	LDI	FP,AR2
	ADDI	18,AR2
*** 153	-----------------------    j = 0u;
	LDI	0,R3
***  	-----------------------    L$2 = 7;
	LDI	7,RC
***	-----------------------g11:
***	-----------------------g77:
	RPTB	L143
*** 154	-----------------------    *U$173++ = *(int*)(j+8912404u);
	LDI	R3,AR0
	LDI	@CONST+53,IR0
	LDI	*+AR0(IR0),R0
	FLOAT	R0
	STF	R0,*AR2++
*** 153	-----------------------    ++j;
L143:	ADDI	1,R3
*** 153	-----------------------    if ( --L$2 >= 0 ) goto g77;
***  	-----------------------    U$307 = (int)delay7_start;
	STI	R6,*+FP(13)
***  	-----------------------    U$298 = (int)delay6_start;
	STI	R7,*+FP(12)
***  	-----------------------    U$289 = (int)delay5_start;
	STI	AR4,*+FP(11)
***  	-----------------------    U$280 = (int)delay4_start;
	STI	AR6,*+FP(10)
***  	-----------------------    U$271 = (int)delay3_start;
	STI	AR7,*+FP(9)
***  	-----------------------    U$262 = (int)delay2_start;
	STI	R5,*+FP(8)
***  	-----------------------    U$252 = (int)delay1_start;
	LDI	*+FP(1),R0
	STI	R0,*+FP(7)
***  	-----------------------    U$239 = (int)delay0_start;
	LDI	*+FP(2),R1
	STI	R1,*+FP(6)
***  	-----------------------    U$201 = in_data_buf1;
	LDI	@_in_data_buf1,R3
	STI	R3,*+FP(4)
***  	-----------------------    U$181 = io_adr;
	LDI	@_io_adr,AR2
***  	-----------------------    U$178 = address_valid_flag_MACRO;
	LDI	@_address_valid_flag_MACRO,R3
	STI	R3,*+FP(17)
L80:
***	-----------------------g14:
*** 176	-----------------------    in_count = 0;
	LDI	0,R0
	STI	R0,*+FP(42)
*** 177	-----------------------    out_count = 0;
	STI	R0,*+FP(43)
*** 179	-----------------------    if ( *(int*)0x87fe0d ) goto g20;
	LDI	@CONST+13,AR0
	LDI	*AR0,R1
	BNZ	L86
*** 179	-----------------------    if ( U$178 ) goto g18;
	LDI	*+FP(17),R1
	BNZ	L84
*** 179	-----------------------    asm("\tSTI\tIOF,@_iflg");
	STI	IOF,@_iflg
*** 179	-----------------------    if ( iflg&0x8 ) goto g20;
	LDI	@_iflg,R3
	TSTB	08h,R3
	BNZ	L86
*** 179	-----------------------    parm_address_MACRO = U$181->xfr_reg;
	LDI	*+AR2(3),R3
	STI	R3,@_parm_address_MACRO
*** 179	-----------------------    address_valid_flag_MACRO = U$178 = 1;
	BD	L86
	LDI	1,R1
	STI	R1,*+FP(17)
	STI	R1,@_address_valid_flag_MACRO
*** 179	-----------------------    goto g20;
***	B	L86	;BRANCH OCCURS
L84:
***	-----------------------g18:
*** 179	-----------------------    asm("\tSTI\tIOF,@_iflg");
	STI	IOF,@_iflg
*** 179	-----------------------    if ( iflg&0x8 ) goto g20;
	LDI	@_iflg,R3
	TSTB	08h,R3
	BNZ	L86
*** 179	-----------------------    parm_MACRO = U$181->xfr_reg;
	LDI	*+AR2(3),R3
*** 179	-----------------------    *(int*)parm_address_MACRO = parm_MACRO;
	LDI	@_parm_address_MACRO,AR1
	STI	R3,*AR1
*** 179	-----------------------    U$178 = address_valid_flag_MACRO = 0;
	STI	R0,@_address_valid_flag_MACRO
	STI	R0,*+FP(17)
L86:
***	-----------------------g20:
*** 18	-----------------------    if ( dma_mode_flag != 1u ) goto g26;
	CMPI	1,IR1
	BNZ	L92
*** 20	-----------------------    if ( data_index != U$116 ) goto g24;
	CMPI	BK,R4
	BNZ	L90
L89:
***	-----------------------g23:
*** 25	-----------------------    if ( ((struct*)0x808000)->transfer_counter ) goto g23;
	LDI	@CONST+64,AR0
	LDI	*AR0,R0
	BNZ	L89
L90:
***	-----------------------g24:
*** 27	-----------------------    if ( ((struct*)0x808000)->transfer_counter ) goto g26;
	LDI	@CONST+64,AR0
	LDI	*AR0,R0
	BNZ	L92
*** 30	-----------------------    *(int*)0x87fe0d = K$121;
	LDI	@CONST+13,AR1
	BD	L93
	LDI	*+FP(5),R0
	STI	R0,*AR1
*** 31	-----------------------    dma_mode_flag = 0u;
	LDI	0,IR1
*** 31	-----------------------    goto g27;
***	B	L93	;BRANCH OCCURS
L92:
***	-----------------------g26:
*** 35	-----------------------    if ( dma_mode_flag ) goto g39;
	CMPI	0,IR1
	BNZ	L105
L93:
***	-----------------------g27:
*** 40	-----------------------    if ( data_index >= U$116 ) goto g35;
	CMPI	BK,R4
	BHS	L101
*** 73	-----------------------    if ( !*(int*)0x87fe0d ) goto g39;
	LDI	@CONST+13,AR0
	LDI	*AR0,R0
	BZ	L105
*** 75	-----------------------    U$181->comd_stat = command0_shadow &= 0xfffffff7u;
	LDI	@_command0_shadow,R0
	ANDN	08h,R0
	STI	R0,@_command0_shadow
	STI	R0,*AR2
L97:
***	-----------------------g31:
*** 100	-----------------------    if ( U$181->comd_stat&0x40 ) goto g31;
	LDI	64,R0
	TSTB	R0,*AR2
	BNZ	L97
*** 102	-----------------------    asm("\tOR\t0001h,IF");
	OR	0001h,IF
*** 103	-----------------------    if ( !first_time_input ) goto g34;
	LDI	*+FP(14),R1
	BZ	L100
*** 105	-----------------------    ((struct*)0x808000)->_gctrl._intval |= 3u;
	LDI	@CONST+62,AR0
	LDI	3,R3
	OR	*AR0,R3
	STI	R3,*AR0
*** 106	-----------------------    first_time_input = 0u;
	LDI	0,R1
	STI	R1,*+FP(14)
L100:
***	-----------------------g34:
*** 108	-----------------------    ((struct*)0x808000)->transfer_counter = U$101;
	BD	L105
	LDI	@CONST+64,AR0
	STI	BK,*AR0
*** 76	-----------------------    dma_mode_flag = 1u;
	LDI	1,IR1
*** 108	-----------------------    goto g39;
***	B	L105	;BRANCH OCCURS
L101:
***	-----------------------g35:
*** 43	-----------------------    U$181->comd_stat = command0_shadow |= 0x8u;
	LDI	@_command0_shadow,R0
	OR	08h,R0
	STI	R0,@_command0_shadow
	STI	R0,*AR2
*** 49	-----------------------    if ( buf_num ) goto g37;
	LDI	*+FP(15),R0
	BNZ	L103
*** 59	-----------------------    read_in_buf = U$122;
	LDI	*+FP(3),R1
	STI	R1,@_read_in_buf
*** 61	-----------------------    ((struct*)0x808000)->destination = U$201;
	LDI	@CONST+65,AR0
	LDI	*+FP(4),R0
	BD	L104
	STI	R0,*AR0
*** 63	-----------------------    buf_num = 1u;
	LDI	1,R1
	STI	R1,*+FP(15)
*** 63	-----------------------    goto g38;
***	B	L104	;BRANCH OCCURS
L103:
***	-----------------------g37:
*** 51	-----------------------    read_in_buf = U$201;
	LDI	*+FP(4),R1
	STI	R1,@_read_in_buf
*** 53	-----------------------    ((struct*)0x808000)->destination = U$122;
	LDI	@CONST+65,AR0
	LDI	*+FP(3),R0
	STI	R0,*AR0
*** 55	-----------------------    buf_num = 0u;
	LDI	0,R1
	STI	R1,*+FP(15)
L104:
***	-----------------------g38:
*** 66	-----------------------    data_index = 0u;
	LDI	0,R4
L105:
***	-----------------------g39:
***  	-----------------------    U$209 = &read_in_buf[data_index];
	LDI	@_read_in_buf,R0
	ADDI	R4,R0,AR4
L107:
***	-----------------------g41:
*** 186	-----------------------    if ( !(*(unsigned*)0x808040&1) ) goto g41;
	LDI	@CONST+14,AR0
	LDI	1,R0
	TSTB	R0,*AR0
	BZ	L107
*** 186	-----------------------    *(unsigned*)0x80804c;
	LDI	@CONST+15,AR1
	NOP	*AR1
*** 186	-----------------------    if ( *(int*)0x87fe04 ) goto g44;
	LDI	@CONST+66,AR1
	LDI	*AR1,R1
	BNZ	L110
*** 186	-----------------------    in_meter1 = *U$209;
	LDI	*AR4,R3
*** 186	-----------------------    in1 = in2 = in_meter1 <<= 16;
	BD	L111
	LSH	16,R3
	FLOAT	R3,R5
	LDF	R5,R6
*** 186	-----------------------    goto g45;
***	B	L111	;BRANCH OCCURS
L110:
***	-----------------------g44:
*** 186	-----------------------    in_meter1 = *U$209;
	LDI	*AR4,RC
*** 186	-----------------------    in_meter2 = in_meter1;
	LDI	RC,R3
*** 186	-----------------------    in1 = in_meter1 <<= 16;
	LSH	16,RC
	FLOAT	RC,R6
*** 186	-----------------------    in2 = in_meter2 &= 0xffff0000;
	ANDN	0ffffh,R3
	FLOAT	R3,R5
L111:
***	-----------------------g45:
*** 188	-----------------------    mute_val_MACRO = *(float*)0x87fe05;
	LDI	@CONST+18,AR1
	LDF	*AR1,R3
*** 188	-----------------------    in1 *= mute_val_MACRO;
	MPYF	R3,R6
*** 188	-----------------------    in2 *= mute_val_MACRO;
	MPYF	R3,R5
***  	-----------------------    U$221 = &old_delay[0];
	LDI	FP,AR4
	ADDI	18,AR4
***  	-----------------------    U$229 = &delay_filtered[0];
	LDI	FP,AR7
	ADDI	34,AR7
*** 192	-----------------------    delay = (int*)0x87fe14;
	LDI	@CONST+53,AR6
***  	-----------------------    L$3 = 7;
	LDI	7,RC
***	-----------------------g47:
***	-----------------------g76:
	RPTB	L142
*** 195	-----------------------    r_tmp_MACRO = (float)*delay++*1.220703125e-4F+*U$221*0.9998779296875F;
	FLOAT	*AR6++,R3
	MPYF	@CONST+67,R3
	LDF	@CONST+68,R0
	MPYF	R0,*AR4,R1
	ADDF	R1,R3
*** 195	-----------------------    *U$221++ = r_tmp_MACRO;
	STF	R3,*AR4++
*** 195	-----------------------    *U$229++ = r_tmp_MACRO;
	FIX	R3,R1
L142:	STI	R1,*AR7++
*** 193	-----------------------    if ( --L$3 >= 0 ) goto g76;
*** 204	-----------------------    rp_MACRO = &ptr0[-delay_filtered[0]];
	LDI	*+FP(34),R1
	SUBI	R1,AR5,AR4
*** 205	-----------------------    if ( (int)rp_MACRO >= U$239 ) goto g50;
	CMPI	*+FP(6),AR4
	BGE	L116
*** 206	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR4
L116:
***	-----------------------g50:
*** 207	-----------------------    dly_out[0] = Y$0 = *rp_MACRO;
	LDF	*AR4,R3
	STF	R3,*+FP(26)
*** 208	-----------------------    *ptr0 = in1+*(float*)0x87fe24*Y$0;
	LDI	@CONST+21,AR0
	MPYF	*AR0,R3
	ADDF	R6,R3
	STF	R3,*AR5
*** 211	-----------------------    tmp_ptr = U$143;
	LDI	*+FP(16),AR4
*** 212	-----------------------    rp_MACRO = &tmp_ptr[-delay_filtered[1]];
	LDI	*+FP(35),AR6
	SUBI	AR6,AR4,AR6
*** 213	-----------------------    if ( (int)rp_MACRO >= U$252 ) goto g52;
	CMPI	*+FP(7),AR6
	BGE	L118
*** 214	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR6
L118:
***	-----------------------g52:
*** 215	-----------------------    dly_out[1] = Y$1 = *rp_MACRO;
	LDF	*AR6,R3
	STF	R3,*+FP(27)
*** 216	-----------------------    *tmp_ptr = in2+*(float*)0x87fe25*Y$1;
	LDI	@CONST+25,AR1
	MPYF	R3,*AR1,R1
	ADDF	R5,R1
	STF	R1,*AR4
*** 220	-----------------------    tmp_ptr += line_len;
	ADDI	R2,AR4
*** 221	-----------------------    rp_MACRO = &tmp_ptr[-delay_filtered[2]];
	LDI	*+FP(36),AR6
	SUBI	AR6,AR4,AR6
*** 222	-----------------------    if ( (int)rp_MACRO >= U$262 ) goto g54;
	CMPI	*+FP(8),AR6
	BGE	L120
*** 223	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR6
L120:
***	-----------------------g54:
*** 224	-----------------------    dly_out[2] = Y$2 = *rp_MACRO;
	LDF	*AR6,R3
	STF	R3,*+FP(28)
*** 225	-----------------------    *tmp_ptr = in1+*(float*)0x87fe26*Y$2;
	LDI	@CONST+29,AR1
	MPYF	R3,*AR1,R1
	ADDF	R6,R1
	STF	R1,*AR4
*** 229	-----------------------    tmp_ptr += line_len;
	ADDI	R2,AR4
*** 230	-----------------------    rp_MACRO = &tmp_ptr[-delay_filtered[3]];
	LDI	*+FP(37),AR6
	SUBI	AR6,AR4,AR6
*** 231	-----------------------    if ( (int)rp_MACRO >= U$271 ) goto g56;
	CMPI	*+FP(9),AR6
	BGE	L122
*** 232	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR6
L122:
***	-----------------------g56:
*** 233	-----------------------    dly_out[3] = Y$3 = *rp_MACRO;
	LDF	*AR6,R3
	STF	R3,*+FP(29)
*** 234	-----------------------    *tmp_ptr = in2+*(float*)0x87fe27*Y$3;
	LDI	@CONST+33,AR1
	MPYF	R3,*AR1,R1
	ADDF	R5,R1
	STF	R1,*AR4
*** 238	-----------------------    tmp_ptr += line_len;
	ADDI	R2,AR4
*** 239	-----------------------    rp_MACRO = &tmp_ptr[-delay_filtered[4]];
	LDI	*+FP(38),AR6
	SUBI	AR6,AR4,AR6
*** 240	-----------------------    if ( (int)rp_MACRO >= U$280 ) goto g58;
	CMPI	*+FP(10),AR6
	BGE	L124
*** 241	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR6
L124:
***	-----------------------g58:
*** 242	-----------------------    dly_out[4] = Y$4 = *rp_MACRO;
	LDF	*AR6,R3
	STF	R3,*+FP(30)
*** 243	-----------------------    *tmp_ptr = in1+*(float*)0x87fe28*Y$4;
	LDI	@CONST+37,AR1
	MPYF	R3,*AR1,R1
	ADDF	R6,R1
	STF	R1,*AR4
*** 247	-----------------------    tmp_ptr += line_len;
	ADDI	R2,AR4
*** 248	-----------------------    rp_MACRO = &tmp_ptr[-delay_filtered[5]];
	LDI	*+FP(39),AR6
	SUBI	AR6,AR4,AR6
*** 249	-----------------------    if ( (int)rp_MACRO >= U$289 ) goto g60;
	CMPI	*+FP(11),AR6
	BGE	L126
*** 250	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR6
L126:
***	-----------------------g60:
*** 251	-----------------------    dly_out[5] = Y$5 = *rp_MACRO;
	LDF	*AR6,R3
	STF	R3,*+FP(31)
*** 252	-----------------------    *tmp_ptr = in2+*(float*)0x87fe29*Y$5;
	LDI	@CONST+41,AR1
	MPYF	R3,*AR1,R1
	ADDF	R5,R1
	STF	R1,*AR4
*** 258	-----------------------    tmp_ptr += line_len;
	ADDI	R2,AR4
*** 259	-----------------------    rp_MACRO = &tmp_ptr[-delay_filtered[6]];
	LDI	*+FP(40),AR6
	SUBI	AR6,AR4,AR6
*** 260	-----------------------    if ( (int)rp_MACRO >= U$298 ) goto g62;
	CMPI	*+FP(12),AR6
	BGE	L128
*** 261	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR6
L128:
***	-----------------------g62:
*** 262	-----------------------    dly_out[6] = Y$6 = *rp_MACRO;
	LDF	*AR6,R3
	STF	R3,*+FP(32)
*** 263	-----------------------    *tmp_ptr = in1+*(float*)0x87fe2a*Y$6;
	LDI	@CONST+45,AR1
	MPYF	R3,*AR1,R1
	ADDF	R6,R1
	STF	R1,*AR4
*** 267	-----------------------    tmp_ptr += line_len;
	ADDI	R2,AR4
*** 268	-----------------------    rp_MACRO = &tmp_ptr[-delay_filtered[7]];
	LDI	*+FP(41),AR6
	SUBI	AR6,AR4,AR6
*** 269	-----------------------    if ( (int)rp_MACRO >= U$307 ) goto g64;
	CMPI	*+FP(13),AR6
	BGE	L130
*** 270	-----------------------    rp_MACRO += line_len;
	ADDI	R2,AR6
L130:
***	-----------------------g64:
*** 271	-----------------------    dly_out[7] = Y$7 = *rp_MACRO;
	LDF	*AR6,R3
	STF	R3,*+FP(33)
*** 272	-----------------------    *tmp_ptr = in2+*(float*)0x87fe2b*Y$7;
	LDI	@CONST+49,AR1
	MPYF	R3,*AR1,R1
	ADDF	R5,R1
	STF	R1,*AR4
*** 276	-----------------------    if ( (++ptr0) >= delay1_start ) goto g66;
	ADDI	1,AR5
	CMPI	*+FP(1),AR5
	BHS	L132
*** 275	-----------------------    ++U$143;
	BD	L133
	LDI	*+FP(16),R1
	ADDI	1,R1
	STI	R1,*+FP(16)
***  	-----------------------    goto g67;
***	B	L133	;BRANCH OCCURS
L132:
***	-----------------------g66:
*** 277	-----------------------    ptr0 = delay0_start;
	LDI	*+FP(2),AR5
***  	-----------------------    U$143 = &ptr0[line_len];
	ADDI	R2,AR5,R1
	STI	R1,*+FP(16)
L133:
***	-----------------------g67:
*** 283	-----------------------    pan_left_gain = (float*)0x87fe34;
	LDI	@CONST+23,AR4
***  	-----------------------    U$321 = pan_left_gain;
*** 282	-----------------------    element_gain = (float*)0x87fe54;
	LDI	@CONST+22,AR6
***  	-----------------------    U$318 = element_gain;
***  	-----------------------    U$319 = &dly_out[0];
	LDI	FP,AR7
	ADDI	26,AR7
*** 184	-----------------------    out1 = 0.0F;
	LDF	0.0,R7
***  	-----------------------    L$4 = 7;
	LDI	7,RC
***	-----------------------g69:
***	-----------------------g75:
	RPTB	L141
*** 285	-----------------------    out1 += (*U$319++ *= *U$318++)**U$321++;
	MPYF	*AR6++,*AR7,R0
	STF	R0,*AR7++
	MPYF	*AR4++,R0
L141:	ADDF	R0,R7
*** 284	-----------------------    if ( --L$4 >= 0 ) goto g75;
*** 290	-----------------------    pan_right_gain = (float*)0x87fe44;
	LDI	@CONST+24,AR4
***  	-----------------------    U$330 = pan_right_gain;
***  	-----------------------    U$328 = &dly_out[0];
	LDI	FP,AR6
	ADDI	26,AR6
*** 184	-----------------------    out2 = 0.0F;
	LDF	0.0,R3
***  	-----------------------    L$5 = 7;
	MPYF	*AR4++,*AR6++,R0
***	-----------------------g72:
***	-----------------------g74:
*** 292	-----------------------    out2 += *U$328++**U$330++;
	RPTS	6
	ADDF	R0,R3
    ||	MPYF	*AR4++,*AR6++,R0
	ADDF	R0,R3
*** 291	-----------------------    if ( --L$5 >= 0 ) goto g74;
*** 295	-----------------------    out1 *= *(float*)0x87fe0b;
	LDI	@CONST+19,AR0
	MPYF	*AR0,R7
*** 295	-----------------------    out2 *= *(float*)0x87fe0b;
	MPYF	*AR0,R3
*** 295	-----------------------    out1 += *(float*)0x87fe0a*in1;
	LDI	@CONST+20,AR1
	MPYF	R6,*AR1,R0
	ADDF	R0,R7
*** 295	-----------------------    out2 += *(float*)0x87fe0a*in2;
	MPYF	R5,*AR1,R0
	ADDF	R0,R3
*** 297	-----------------------    i_val1_MACRO = out1;
	FIX	R7,RC
*** 299	-----------------------    i_val2_MACRO = out2;
	FIX	R3
*** 299	-----------------------    out_meter1_dma += ABS(i_val1_MACRO >>= 16);
	ASH	-16,RC
	ABSI	RC,R0
	ADDI	@_out_meter1_dma,R0
	STI	R0,@_out_meter1_dma
*** 299	-----------------------    i_val1_MACRO <<= 16;
	LSH	16,RC
*** 299	-----------------------    out_meter2_dma += ABS(i_val2_MACRO >>= 16);
	ASH	-16,R3
	ABSI	R3,R1
	ADDI	@_out_meter2_dma,R1
	STI	R1,@_out_meter2_dma
*** 299	-----------------------    i_val1_MACRO |= i_val2_MACRO &= 0xffff;
	AND	0ffffh,R3
	OR	R3,RC
*** 299	-----------------------    *(unsigned*)0x808048 = i_val1_MACRO;
	LDI	@CONST+16,AR1
	STI	RC,*AR1
*** 299	-----------------------    data_index = (data_index < U$116) ? data_index+1u : data_index;
	CMPI	BK,R4
	BHS	L80
	ADDI	1,R4
*** 299	-----------------------    goto g14;
	B	L80
	.globl	_T3x_hinit
******************************************************
* FUNCTION DEF : _T3x_hinit
******************************************************
_T3x_hinit:
	.globl	_c_int00
	.globl	_c_int01
	.globl	_c_int02
	.globl	_c_int03
*** 130	-----------------------    *(unsigned*)0x808064 = 5177352u;
	LDI	@CONST+69,AR0
	LDI	@CONST+70,R0
	STI	R0,*AR0
*** 131	-----------------------    *(unsigned*)0x808068 = 983048u;
	LDI	@CONST+71,AR1
	LDI	@CONST+72,R1
	STI	R1,*AR1
*** 133	-----------------------    *(unsigned*)0x808060 = 40u;
	LDI	@CONST+73,AR2
	LDI	40,R2
	STI	R2,*AR2
*** 142	-----------------------    asm("\tOR\t4800h,ST");
	OR	4800h,ST
*** 144	-----------------------    asm("\tANDN\t2000h,ST");
	ANDN	2000h,ST
*** 146	-----------------------    asm("\tLDI\t0,IOF");
	LDI	0,IOF
*** 149	-----------------------    asm("\tANDN\t0FFFDh,IF");
	ANDN	0FFFDh,IF
*** 150	-----------------------    asm("\tOR  \t0002h,IF");
	OR  	0002h,IF
*** 152	-----------------------    putchr((-1515870811));
	LDI	@CONST+74,R3
	PUSH	R3
	CALL	_putchr
	SUBI	1,SP
*** 154	-----------------------    putchr(1515870810);
	LDI	@CONST+75,R0
	PUSH	R0
	CALL	_putchr
	SUBI	1,SP
***  	-----------------------    return;
EPI0_8:
	RETS
******************************************************
* DEFINE CONSTANTS                                   *
******************************************************
	.bss	CONST,76
	.sect	".cinit"
	.word	76,CONST
	.word 	126976           ;0
	.word 	9437184          ;1
	.word 	65538            ;2
	.word 	07fffff00h       ;3
	.word 	8912384          ;4
	.word 	135168           ;5
	.word 	9568256          ;6
	.word 	8912387          ;7
	.word 	3932160          ;8
	.word 	50000            ;9
	.word 	205258752        ;10
	.word 	040000h          ;11
	.word 	0400004h         ;12
	.word 	8912397          ;13
	.word 	8421440          ;14
	.word 	8421452          ;15
	.word 	8421448          ;16
	.word 	0fffbffffh       ;17
	.word 	8912389          ;18
	.word 	8912395          ;19
	.word 	8912394          ;20
	.word 	8912420          ;21
	.word 	8912468          ;22
	.word 	8912436          ;23
	.word 	8912452          ;24
	.word 	8912421          ;25
	.word 	8912469          ;26
	.word 	8912437          ;27
	.word 	8912453          ;28
	.word 	8912422          ;29
	.word 	8912470          ;30
	.word 	8912438          ;31
	.word 	8912454          ;32
	.word 	8912423          ;33
	.word 	8912471          ;34
	.word 	8912439          ;35
	.word 	8912455          ;36
	.word 	8912424          ;37
	.word 	8912472          ;38
	.word 	8912440          ;39
	.word 	8912456          ;40
	.word 	8912425          ;41
	.word 	8912473          ;42
	.word 	8912441          ;43
	.word 	8912457          ;44
	.word 	8912426          ;45
	.word 	8912474          ;46
	.word 	8912442          ;47
	.word 	8912458          ;48
	.word 	8912427          ;49
	.word 	8912475          ;50
	.word 	8912443          ;51
	.word 	8912459          ;52
	.word 	8912404          ;53
	.word 	8912405          ;54
	.word 	8912406          ;55
	.word 	8912407          ;56
	.word 	8912408          ;57
	.word 	8912409          ;58
	.word 	8912410          ;59
	.word 	8912411          ;60
	.word 	32768            ;61
	.word 	8421376          ;62
	.word 	8421392          ;63
	.word 	8421384          ;64
	.word 	8421382          ;65
	.word 	8912388          ;66
	.float	1.220703125e-4   ;67
	.float	9.998779296875e-1;68
	.word 	8421476          ;69
	.word 	5177352          ;70
	.word 	8421480          ;71
	.word 	983048           ;72
	.word 	8421472          ;73
	.word 	-1515870811      ;74
	.word 	1515870810       ;75
******************************************************
* UNDEFINED REFERENCES                               *
******************************************************
	.globl	DIV_U30
	.globl	DIV_I30
	.end
