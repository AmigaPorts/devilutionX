* ----------------------------------------------------------------------------------------------
* diablo.s -- replacement of C code by hand-written asm code by S.Devulder
* ----------------------------------------------------------------------------------------------
    section .text

BUFFER_WIDTH    set     768      ; FIXME: set the correct value here

    XDEF    _RenderTile_RT_SQUARE
    XDEF    _RenderTile_RT_TRANSPARENT
    XDEF    _RenderTile_RT_LTRIANGLE
    XDEF    _RenderTile_RT_RTRIANGLE
    XDEF    _RenderTile_RT_LTRAPEZOID
    XDEF    _RenderTile_RT_RTRAPEZOID

	XDEF    _RenderLine0
	XDEF    _RenderLine1
	XDEF    _RenderLine2

    XREF    __ZN3dvl10gpBufStartE
    XREF    __ZN3dvl8gpBufEndE

    cnop    0,4

* ----------------------------------------------------------------------------------------------

* inline static void RenderLine(BYTE **dst, BYTE **src, int n, BYTE *tbl, DWORD mask)
* a0 = *dst
* a1 = *src
* d0 = n    (1..32)
* a2 = tbl
* d1 = *mask 
* d2 = scratch
* d3 = scratch
* CC sets according to d1 value

inc_a0 macro
    addq.l  #1,a0
    endm

inc_a0_a1 macro
*   cmp.b   (a1)+,(a0)+
    addq.l  #1,a1
    addq.l  #1,a0
    endm

unroll macro
    btst    #5,d0
    beq     \1_4
    \1
    \1
    \1
    \1
    \1
    \1
    \1
    \1
\1_4
    btst    #4,d0
    beq     \1_2
    \1
    \1
    \1
    \1
\1_2
    btst    #3,d0
    beq     \1_1
    \1
    \1
\1_1
    btst    #2,d0
    beq     \2_1
    \1
\2_1
    btst    #1,d0
    beq     \3_1
    \2
\3_1
    btst    #0,d0
    beq     \3_2
    \3
\3_2
	rts
	endm
	
loop macro
\1__1
    add.l   d1,d1
    bcs     \1__2
    \1
    subq.w  #1,d0
    bne     \1__1
    rts
\1__2
    \2
    subq.w  #1,d0
    bne     \1__1
	rts
    endm

* case light_table_index == 0
_RenderLine0_
	add.w	d0,a1
    add.w   d0,a0
	rts
_RenderLine0
    move.l  -(a3),d1
	beq.b	_RenderLine0_
    cmp.l   a5,a0
    bcs.b   _RenderLine0_
    cmp.l   a6,a0
    bhi.b   _RenderLine0_
	
	not.l	d1
	bne		.l1
.m4 macro
    move.l  (a1)+,(a0)+
    endm
.m2 macro
    move.w  (a1)+,(a0)+
    endm
.m1 macro
    move.b  (a1)+,(a0)+
    endm
    unroll  .m4,.m2,.m1

.l1
	cmp.l	#$AAAAAAAA,d1			; bg / fg / bg fg
	bne		.l2
.p4	macro
    ifeq 1
	move.b	1(a1),1(a0)				; 2
	move.b	3(a1),3(a0)				; 2
	addq.l	#4,a1					; .5
	addq.l	#4,a0					; .5 ==> 5 cycles
	else
	move.l	(a1)+,d1				; \
	and.l	#$00FF00FF,d1			; / 1 cycle (fused) ?
	move.l	(a0),d2					; \
	and.l	#$FF00FF00,d2			; / 1 cycle (fused) ?
	or.l	d2,d1					; 1
	move.l	d1,(a0)+				; 1 ==> 4 cycles
	endc
	endm
.p2	macro
	move.b	1(a1),1(a0)
	addq.l	#2,a1
	addq.l	#2,a0
	endm
.p1 macro
	addq.l	#1,a1
	addq.l	#1,a0
	endm
	unroll	.p4,.p2,.p1

.l2
	cmp.l	#$55555555,d1			; fg / bg /fg / bg
	bne		.l3

.q4	macro
	ifeq 1
	move.b	(a1),(a0)				; 2
	move.b	2(a1),2(a0)				; 2
	addq.l	#4,a1					; .5
	addq.l	#4,a0					; .5 ==> 5 cycles
	else
	move.l	#$FF00FF00,d1
	and.l	(a1)+,d1
	move.l	#$00FF00FF,d2
	and.l	(a0),d2
	or.l	d2,d1
	move.l	d1,(a0)+
	endc
	endm
.q2	macro
	ifeq 0
	move.b	(a1),(a0)
	addq.l	#2,a1
	addq.l	#2,a0
	else
	move.w	#$FF00,d1
	and.w	(a1)+,d1
	move.w	#$00FF,d2
	and.w	(a0),d2
	or.w	d2,d1
	move.w	d1,(a0)+
	endc
	endm
.q1 macro
	move.b	(a1)+,(a0)+
	endm
	unroll	.q4,.q2,.q1

.l3
	loop	.m1,inc_a0_a1


* case light_table_index == lightmax
_RenderLine1_
	add.w	d0,a0
	rts
_RenderLine1
    move.l  -(a3),d1
    add.w   d0,a1
	beq.b	_RenderLine1_
    cmp.l   a5,a0
    bcs.b   _RenderLine1_
    cmp.l   a6,a0
    bhi.b   _RenderLine1_
	
	not.l	d1
	bne		.l1
	
.m4 macro
    clr.l	(a0)+
    endm
.m2 macro
	clr.w  	(a0)+
    endm
.m1 macro
    clr.b  	(a0)+
    endm
    unroll  .m4,.m2,.m1
.l1
	cmp.l	#$AAAAAAAA,d1
	bne		.l2
	move.l	#$FF00FF00,d2
.p4	macro
	and.l	d2,(a0)+
	endm
.p2	macro
	and.w	d2,(a0)+
	endm
.p1 macro
	addq.l	#1,a0
	endm
	unroll	.p4,.p2,.p1

.l2
	cmp.l	#$55555555,d1
	bne		.l3
	move.l	#$00FF00FF,d2
.q4	macro
	and.l	d2,(a0)+
	endm
.q2	macro
	and.w	d2,(a0)+
	endm
.q1 macro
	clr.b	(a0)+
	endm
	unroll	.q4,.q2,.q1
.l3
	loop	.m1,inc_a0

* other cases
_RenderLine2_
	add.w	d0,a1
	add.w	d0,a0
	rts
_RenderLine2
    move.l  -(a3),d1
	beq		_RenderLine2_
    cmp.l   a5,a0
    bcs.b   _RenderLine2_
    cmp.l   a6,a0
    bhi.b   _RenderLine2_

	moveq   #0,d2
	moveq   #0,d3
	
	
	not.l	d1
	bne		.l1
.m4 macro
    move.b  (a1)+,d2		; \ merged ?
    move.b  (a1)+,d3		; / 
    move.w  (a2,d2.w),d1
    move.b  (a2,d3.w),d1
    swap    d1
    move.b  (a1)+,d2		; \
    move.b  (a1)+,d3		; /
    move.w  (a2,d2.w),d1
    move.b  (a2,d3.w),d1
    move.l  d1,(a0)+
    endm
.m2 macro
    move.b  (a1)+,d2		; \
    move.b  (a1)+,d3		; /
    move.w  (a2,d2.w),d1
    move.b  (a2,d3.w),d1
    move.w  d1,(a0)+
    endm
.m1 macro
    move.b  (a1)+,d2
    move.b  (a2,d2.w),(a0)+
    endm
	unroll  .m4,.m2,.m1

.l1
	cmp.l	#$AAAAAAAA,d1
	bne		.l2
.p4	macro
    move.b  1(a1),d2
    move.b  3(a1),d3
    move.b  (a2,d2.w),1(a0)
    move.b  (a2,d3.w),3(a0)
	addq.l	#4,a1
	addq.l	#4,a0
    endm
.p2 macro
    move.b  1(a1),d2
    move.b  (a2,d2.w),1(a0)
	addq.l	#2,a1
	addq.l	#2,a0
    endm
.p1 macro
	inc_a0_a1
	endm
	unroll  .p4,.p2,.p1
	
.l2
	cmp.l	#$55555555,d1
	bne		.l3	
.q4 macro
    move.b  (a1),d2
    move.b  2(a1),d3
    move.b  (a2,d2.w),(a0)
    move.b  (a2,d3.w),2(a0)
	addq.l	#4,a1
	addq.l	#4,a0
    endm
.q2 macro
    move.b  (a1),d2
    move.b	(a2,d2.l),(a0)
	addq.l	#2,a1
	addq.l	#2,a0
	endm
.q1 macro
    move.b  (a1)+,d2
    move.b  (a2,d2.l),(a0)+
    endm
	unroll  .q4,.q2,.q1	

.l3
	loop	.m1,inc_a0_a1

*----------------------------------------------------------------------------------------
setup macro
* get params from stack
*   movem.l (4*(1+\1),sp),a0/a1/a2/a3
    move.l  (4*(1+\1),sp),a0    ; \
    move.l  (4*(2+\1),sp),a1    ; / fused

    move.l  (4*(3+\1),sp),a2    ; \ fused
    move.l  (4*(4+\1),sp),a3    ; /

* determine renderFcn
	lea		_RenderLine0(pc),a4
    move.l  __ZN3dvl17light_table_indexE,d2
    beq     .L0
    sub.b   __ZN3dvl8lightmaxE,d2
	lea		_RenderLine2(pc),a4
    bne.b   .L0
	lea		_RenderLine1(pc),a4
* factorize constants in regs
.L0
    move.l  __ZN3dvl10gpBufStartE,a5
    addq.l  #4,a3
    move.l  __ZN3dvl8gpBufEndE,a6
    endm

prologue_7 macro
*    movem.l d2-d3/a2-a6,-(sp)
    sub.w   #4*7,sp
    move.l  d2,4*0(sp)      ; \
    move.l  d3,4*1(sp)      ; / fused

    move.l  a2,4*2(sp)      ; \
    move.l  a3,4*3(sp)      ; / fused

    move.l  a4,4*4(sp)      ; \
    move.l  a5,4*5(sp)      ; / fused

    move.l  a6,4*6(sp)
    setup   7
    endm

epilogue_7  macro
*   movem.l (sp)+,d2-d3/a2-a3/a4-a6
    move.l  (sp)+,d2        ; \ fused
    move.l  (sp)+,d3        ; /
    move.l  (sp)+,a2        ; \ fused
    move.l  (sp)+,a3        ; /
    move.l  (sp)+,a4        ; \ fused
    move.l  (sp)+,a5        ; /
    move.l  (sp)+,a6        ; \ fused??
    rts                     ; /
    endm

prologue_11 macro
*    movem.l d2-d7/a2-a6,-(sp)
    sub.w   #4*11,sp
    move.l  d2,4*0(sp)      ; \
    move.l  d3,4*1(sp)      ; / fused

    move.l  d4,4*2(sp)      ; \
    move.l  d5,4*3(sp)      ; / fused

    move.l  d6,4*4(sp)      ; \
    move.l  d7,4*5(sp)      ; / fused

    move.l  a2,4*6(sp)      ; \
    move.l  a3,4*7(sp)      ; / fused

    move.l  a4,4*8(sp)      ; \
    move.l  a5,4*9(sp)      ; / fused

	move.l	a6,4*10(sp)
	
    setup   11
    endm

epilogue_11 macro
*    movem.l (sp)+,d2-d7/a2-a6
    move.l  (sp)+,d2        ; \ fused
    move.l  (sp)+,d3        ; /
    move.l  (sp)+,d4        ; \ fused
    move.l  (sp)+,d5        ; /
    move.l  (sp)+,d6        ; \ fused
    move.l  (sp)+,d7        ; /
    move.l  (sp)+,a2        ; \ fused
    move.l  (sp)+,a3        ; /
    move.l  (sp)+,a4        ; \ fused
    move.l  (sp)+,a5        ; /
    move.l  (sp)+,a6        ; \ fused??
    rts                     ; /
    endm

*----------------------------------------------------------------------------------------
* extern void RenderTile_RT_TRANSPARENT(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_TRANSPARENT
    prologue_11
	addq.l	#2,a4			; skip over load mask
    moveq   #31,d7
.L1
    move.l  -(a3),d6        ; m = *mask; mask--
    moveq   #32,d4
.L2
    move.b  (a1)+,d5        ;  v = *src++;
    ext.w   d5
    bgt.b   .L3
    suba.w  d5,a0           ; dst += (-v)
    neg.w   d5              ; v =-v (parallel!)
    bra.b   .L4
.L3
    move.w  d5,d0
    move.l  d6,d1
    jsr     (a4)
.L4
    lsl.l   d5,d6           ; m <<= v
    sub.w   d5,d4           ; j -= v
    bne.b   .L2
    sub.w   #BUFFER_WIDTH+32,a0
    dbra    d7,.L1
    epilogue_11

*----------------------------------------------------------------------------------------
* extern void RenderTile_RT_SQUARE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_SQUARE
    prologue_7
	bsr		block16
	bsr		block16
	epilogue_7
	
*----------------------------------------------------------------------------------------

	XDEF	block16
block16
	REPT	16
	moveq	#32,d0
    jsr     (a4)
    sub.w   #BUFFER_WIDTH+32,a0
    ENDR
	rts
	
triangL
.i	set		30
	add.w	#.i,a0
	REPT  	16
	IFNE	.i&2
	addq.w	#2,a1
	ENDC
    moveq   #32-.i,d0
    jsr     (a4)
	IFNE	.i
.i	set		.i-2
	ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0	
	ENDR
	rts

triangR
.i	set		30
	REPT    16
    moveq   #32-.i,d0
    jsr     (a4)
	IFNE	.i&2
	addq.w	#2,a1
	ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
.i	set		.i-2
	ENDR
	rts

*----------------------------------------------------------------------------------------
* extern void RenderTile_RT_LTRIANGLE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_LTRIANGLE
    prologue_7
	bsr		triangL
.i	set		2
	addq.l	#.i,a0
	REPT  	15
	IFNE	.i&2
	addq.w	#2,a1
	ENDC
    moveq   #32-.i,d0
    jsr     (a4)
	IFNE	.i-30
.i	set		.i+2
	sub.w   #BUFFER_WIDTH+32-.i,a0
	ENDC
	ENDR	
    epilogue_7

*----------------------------------------------------------------------------------------
* extern void RenderTile_RT_RTRIANGLE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_RTRIANGLE
    prologue_7
	bsr		triangR
.i	set		2
	REPT    15
    moveq   #32-.i,d0
    jsr     (a4)
	IFNE	.i&2
	addq.w	#2,a1
	ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
.i	set		.i+2
	ENDR	
    epilogue_7

*----------------------------------------------------------------------------------------
* extern void RenderTile_RT_LTRAPEZOID(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_LTRAPEZOID
    prologue_7
	bsr		triangL
	bsr		block16
    epilogue_7

*----------------------------------------------------------------------------------------
* extern void RenderTile_RT_RTRAPEZOID(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_RTRAPEZOID
    prologue_7
	bsr		triangR
	bsr		block16
    epilogue_7

* end of file