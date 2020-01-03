* -----------------------------------------------------------------------------
* render68k.asm -- replacement of C code by hand-written asm code by S.Devulder
* -----------------------------------------------------------------------------
    machine 68080

    section .text

BUFFER_WIDTH    set     768
BYTE_INDEX_MODE set     0   ; 1 = experimental index mode on v4
INLINE_BLOCK16  set     1   ; 1 seem faster, but hard to tell

    XDEF    _RenderTile_RT_SQUARE
    XDEF    _RenderTile_RT_TRANSPARENT
    XDEF    _RenderTile_RT_LTRIANGLE
    XDEF    _RenderTile_RT_RTRIANGLE
    XDEF    _RenderTile_RT_LTRAPEZOID
    XDEF    _RenderTile_RT_RTRAPEZOID

    XDEF    _RenderLine0
    XDEF    _RenderLine1
    XDEF    _RenderLine2
    XDEF    _RenderLine0_AMMX
    XDEF    _RenderLine1_AMMX
    XDEF    _RenderLine2_AMMX

    XREF    __ZN3dvl10gpBufStartE
    XREF    __ZN3dvl8gpBufEndE

*   XREF    __ZN3dvl8lightmaxE
    XREF    __ZN3dvl17light_table_indexE
    XREF    _ac68080_ammx

    cnop    0,4

bank macro
    inline
.aa equ   *
    dc.w    (%0111000100000000+((\1)*%100)+(\2)+((.bb)*%1000000))
    ifb   \5
      \3    \4
    else
      \3    \4,\5
  endc
.bb equ   (*-.aa-4)>>1
    einline
  endm

* -----------------------------------------------------------------------------
* inline static void RenderLine(BYTE **dst, BYTE **src, int n, BYTE *tbl, DWORD mask)
* a0 = *dst
* a1 = *src
* d0 = n    (1..32)
* a2 = tbl
* d1 = *mask
* d2 = scratch
* d3 = scratch
* d4 = scratch
* d6 = scratch

_RenderLine_NONE
    add.w   d0,a1
    add.w   d0,a0
    rts

chk_bounds  macro
.ck cmpa.l  a5,a0
    bcc     _RenderLine_NONE
    addq.l  #.ok_bounds-.ck,a4
.ok_bounds
    endm

* -----------------------------------------------------------------------------
* AMMX version

* binary search from 32 up to 7
binAMMX macro
    inline
    bclr    #5,d0
    beq     .l16_31
* 32 bytes in a row
    \1
    \1
    \1
    \1
    rts
.l16_31
* 16 to 31 bytes
    bclr    #4,d0
    beq     .l8_15
    \1
    \1
.l8_15
* 8 to 15 bytes
    bclr    #3,d0
    beq     .l0_7
* 8 bytes in a row
    \1
.l0_7
* 0 to 7 bytes
    \2
* fixup ptrs
    ifne    \3&%01
    add.w   d0,a0
    endc
    ifne    \3&%10
    add.w   d0,a1
    endc
    rts
    einline
    endm

* -----------------------------------------------------------------------------
* case light_table_index == lightmax

_RenderLine1_AMMX
    chk_bounds
.n8 macro
    store   d2,(a0)+
    endm
.n0 macro
    storec  d2,d0,(a0)
    endm
.m8 macro
    rol.l   #8,d1
    storem  d2,d1,(a0)+
    endm
.m0 macro
    bfclr   d1{d0:8}
    rol.l   #8,d1
    storem  d2,d1,(a0)
    endm

    peor    d2,d2,d2    ; d2=0.q
    moveq   #1,d3
    add.l   d1,d3
    add.l   d0,a1       ; advance
    bne     .mask
    binAMMX .n8,.n0,%01
.mask
    binAMMX .m8,.m0,%01

* -----------------------------------------------------------------------------
* case light_table_index == 0

_RenderLine0_AMMX
    chk_bounds
.n8 macro
    load    (a1)+,d2
    store   d2,(a0)+
    endm
.n0 macro
    load    (a1),d2
    storec  d2,d0,(a0)
    endm
.m8 macro
    rol.l   #8,d1
    load    (a1)+,d2
    storem  d2,d1,(a0)+
    endm
.m0 macro
    load    (a1),d2
    bfclr   d1{d0:8}
    rol.l   #8,d1
    storem  d2,d1,(a0)
    endm

    moveq   #1,d3
    add.l   d1,d3
    bne     .mask
    binAMMX .n8,.n0,%11
.mask
    binAMMX  .m8,.m0,%11

* -----------------------------------------------------------------------------
* other cases

move_ macro
; move.\1  (a2,d\2 . b\3),d\4
    dc.w    %0000000000110010+%0001000000000000*\1+%0000001000000000*\4
    dc.w    %0000000100001000+%0001000000000000*\2+\3
    endm

transform   macro
*   move.l  (a1)+,d3        ; F(used)  d3=AABBCCDD
*   move.l  (a1)+,d5        ; F  1
* input d3/d5
  ifeq  BYTE_INDEX_MODE
    move.l  d3,d2           ; p1      d3=AABBCCDD
    rol.l   #8,d2           ; p1      d2=BBCCDDAA
    move.l  d5,d4           ; p2
    rol.l   #8,d4           ; p2 2
    swap    d3              ; p1      d3=CDCDDAABB
    and.l   #$00FF00FF,d2   ; p2 3    d2=00CC00AA
    swap    d5              ; p1
    and.l   #$00FF00FF,d3   ; p2 4    d3=00DD00BB
    and.l   #$00FF00FF,d4   ; p1
    and.l   #$00FF00FF,d5   ; p2 5
    move.w  (a2,d2.w),d2    ; p1 6    d2=00CCxx--
    move.b  (a2,d3.w),d2    ; p1      d2=00CCxxyy
    swap    d2              ; p2 7    d2=xxyy00CC
    swap    d3              ; p1      d3=00BB00DD
    move.w  (a2,d4.w),d4    ; p2 8
    move.b  (a2,d5.w),d4    ; p1
    swap    d4              ; p2 9
    swap    d5              ; p1
    move.w  (a2,d2.w),d2    ; p2 10   d2=xxyyzz--
    move.b  (a2,d3.w),d2    ; p1 11   d2=xxyyzztt
    move.w  (a2,d4.w),d4    ; p1 12
    move.b  (a2,d5.w),d4    ; p1 13
  else    ; BYTE_INDEX_MODE
    move_   %11,3,3,2       ; p1 4  move.w  (a2,d3.b3),d2
    move_   %01,3,2,2       ; p1 5  move.b  (a2,d3.b2),d2
    move_   %11,5,3,4       ; p1 6  move.w  (a2,d5.b3),d4
    swap    d2              ; p2 6
    move_   %01,5,2,4       ; p1 7  move.b  (a2,d5.b2),d4
    move_   %11,3,1,2       ; p1 8  move.w  (a2,d3.b1),d2
    swap    d4              ; p2 8
    move_   %01,3,0,2       ; p1 9  move.b  (a2,d3.b0),d2
    move_   %11,5,1,4       ; p1 10 move.w  (a2,d5.b1),d4
    move_   %01,5,0,4       ; p1 11 move.b  (a2,d5.b0),d4
  endc    ; BYTE_INDEX_MODE
* output d2/d4
*   move.l  d2,(a0)+        ; F
*   move.l  d4,(a0)+        ; F  12 ==> 12 cycles for 8 pixels ?
  endm

transfAA55_8 macro
  ifeq  BYTE_INDEX_MODE
    move.b  \1(a1),d2               ; 1
    move.b  \1+2(a1),d3             ; 2
    move.b  \1+4(a1),d4             ; 3
    move.b  \1+6(a1),d5             ; 4
    addq.l  #8,a0                   ; 4
    addq.l  #8,a1                   ; 5
    move.w  (a2,d2.w),d1            ; 5
    move.b  (a2,d3.w),d1            ; 6
    swap    d1                      ; 7
    move.w  (a2,d4.w),d1            ; 8
    move.b  (a2,d5.w),d1            ; 9
    movep.l d1,\1-8(a0)             ; 10
  else ; BYTE_INDEX_MODE
    move.l  \1(a1),d2               ; 1 F
    move.l  \1+4(a1),d4             ; 1 F
    move_   %11,2,3,1               ; 4 move.w  (a2,d2.b3),d1
    move_   %01,2,1,1               ; 5 move.b  (a2,d2.b1),d1
    addq.l  #8,a1                   ; 5
    swap    d1                      ; 6
    addq.l  #8,a0                   ; 6
    move_   %11,4,3,1               ; 7 move.w  (a2,d4.b3),d1
    move_   %01,4,1,1               ; 8 move.b  (a2,d4.b1),d1
    movep.l d1,\1-8(a0)             ; 9
  endc ; BYTE_INDEX_MODE
  endm

transfAA55 macro
    inline
  ifeq  BYTE_INDEX_MODE
    moveq   #0,d2
    moveq   #0,d3
    moveq   #0,d4
    moveq   #0,d5
  endc
    bclr    #5,d0
    beq     .b4
    transfAA55_8 \1
    transfAA55_8 \1
    transfAA55_8 \1
    transfAA55_8 \1
    rts
.b4
    bclr    #4,d0
    beq.b   .b3
    transfAA55_8 \1
    transfAA55_8 \1
.b3
    bclr    #3,d0
    beq.b   .b2
    transfAA55_8 \1
.b2
    bclr    #2,d0
    beq.b   .b1
    move.b  \1(a1),d2     ; 1
    move.b  \1+2(a1),d3   ; 2
    addq.l  #4,a0         ; 2
    addq.l  #4,a1         ; 3
    move.w  (a2,d2.w),d1  ; 3+1
    move.b  (a2,d3.w),d1  ; 5
    movep.w d1,\1-4(a0)   ; 6
.b1
    bclr    #1,d0
    beq.b   .b0
    move.b  \1(a1),d2     ; 1
    addq.l  #2,a0
    addq.l  #2,a1         ; 2
; 2 bubbles
    move.b  (a2,d2.w),d1  ; 4
    move.b  d1,\1-2(a0)   ; 5
.b0
  ifeq  \1
    tst.b   d0
    beq.b   .bb0
    move.b  (a1)+,(a0)+
.bb0
  else
    add.w   d0,a0
    add.w   d0,a1
  endc
    rts
    einline
  endm

_RenderLine2_AMMX
    chk_bounds
.n8 macro
    move.l  (a1)+,d3
    move.l  (a1)+,d5
    transform
    move.l  d2,(a0)+
    move.l  d4,(a0)+
  endm
.n0 macro
    move.l  (a1),d3
    move.l  4(a1),d5
    transform
    vperm   #$4567CDEF,d2,d4,d2
    storec  d2,d0,(a0)
  endm
.m8 macro
    move.l  (a1)+,d3        ; F(used)  d3=AABBCCDD
    move.l  (a1)+,d5        ; F  1
    rol.l   #8,d1
    transform
    vperm   #$4567CDEF,d2,d4,d2
    storem  d2,d1,(a0)+
    endm
.m0 macro
    bfclr   d1{d0:8}
    move.l  (a1),d3         ; F(used)  d3=AABBCCDD
    move.l  4(a1),d5        ; F  1
    transform
    vperm   #$4567CDEF,d2,d4,d2
    rol.l   #8,d1
    storem  d2,d1,(a0)
    endm

    move.l  d1,d3           ; \ fused
    addq.l  #1,d3           ; /
    bne     .mask
    binAMMX .n8,.n0,%11
.maskAA
    transfAA55   0
.mask55
    transfAA55   1
.mask
    move.l  #$AAAAAAAA,d3
    eor.l   d1,d3
    beq     .maskAA
    not.l   d3
    beq     .mask55
.maskXX
    binAMMX  .m8,.m0,%11

* -----------------------------------------------------------------------------

inc_a0 macro
    addq.l  #1,a0
    endm

inc_a0_a1 macro
    cmp.b   (a1)+,(a0)+
*    addq.l  #1,a0
*    addq.l  #1,a1
    endm

bin68k macro
    inline
    bclr  #5,d0
    beq   .l16_31
    \1
    \1
    \1
    \1
    \1
    \1
    \1
    \1
    rts
.l16_31
    bclr  #4,d0
    beq   .l8_15
    \1
    \1
    \1
    \1
.l8_15
    bclr  #3,d0
    beq   .l4_7
    \1
    \1
.l4_7
    bclr  #2,d0
    beq   .l2_3
    \1
.l2_3
    bclr  #1,d0
    beq   .l1
    \2
.l1
    tst.b d0
    beq   .l0
    \3
.l0
    einline
    rts
    endm

msk68k macro
    inline
.l1
    add.l   d1,d1
    bcs     .l2
    \1
    subq.l  #1,d0
    bne     .l1
    rts
.l2
    \2
    subq.l  #1,d0
    bne     .l1
    einline
    rts
    endm

* -----------------------------------------------------------------------------
* case light_table_index == 0

_RenderLine0
    chk_bounds
.m4 macro
    move.l  (a1)+,(a0)+
    endm
.m2 macro
    move.w  (a1)+,(a0)+
    endm
.m1 macro
    move.b  (a1)+,(a0)+
    endm
.a4 macro
    move.b  1(a1),1(a0)             ; 2
    addq.l  #4,a1                   ; .5
    move.b  -1(a1),3(a0)            ; 2
    addq.l  #4,a0                   ; .5 ==> 5 cycles
    endm
.a2 macro
    move.b  1(a1),d1
    addq.l  #2,a1
    move.b  d1,1(a0)
    addq.l  #2,a0
    endm
.a1 macro
    addq.l  #1,a0
    addq.l  #1,a1
    endm
.b4 macro
    move.b  (a1),(a0)               ; 2
    addq.l  #4,a1                   ; .5
    move.b  -2(a1),2(a0)            ; 2
    addq.l  #4,a0                   ; .5 ==> 5 cycles
    endm
.b2 macro
    move.b  (a1),d1
    addq.l  #2,a1
    move.b  d1,(a0)
    addq.l  #2,a0
    endm
.b1 macro
    move.b  (a1)+,(a0)+
    endm

    not.l   d1
    bne     .mask
    bin68k  .m4,.m2,.m1
.mask
    cmp.l   #$AAAAAAAA,d1           ; bg / fg / bg fg
    bne     .l2
    bin68k  .a4,.a2,.a1
.l2
    cmp.l   #$55555555,d1           ; fg / bg /fg / bg
    bne     .l3
    bin68k  .b4,.b2,.b1
.l3
    msk68k  .m1,inc_a0_a1

* -----------------------------------------------------------------------------
* case light_table_index == lightmax

_RenderLine1
    chk_bounds
.m4 macro
    clr.l   (a0)+
    endm
.m2 macro
    clr.w   (a0)+
    endm
.m1 macro
    clr.b   (a0)+
    endm
.p4 macro
    and.l   d2,(a0)+
    endm
.p2 macro
    and.w   d2,(a0)+
    endm
.p1 macro
    and.b   d2,(a0)+
    endm

    not.l   d1
    add.w   d0,a1
    bne     .mask
    bin68k  .m4,.m2,.m1
.mask
    cmp.l   #$AAAAAAAA,d1
    bne     .l2
    move.l  #$FF00FF00,d2
    bin68k  .p4,.p2,.p1
.l2
    cmp.l   #$55555555,d1
    bne     .l3
    move.l  #$00FF00FF,d2
    bin68k  .p4,.p2,.p1
.l3
    msk68k  .m1,inc_a0

* -----------------------------------------------------------------------------
* other cases
_RenderLine2
    chk_bounds
.m4 macro
    move.b  (a1)+,d2        ; \ merged ?
    move.b  (a1)+,d3        ; /
    move.w  (a2,d2.w),d1
    move.b  (a2,d3.w),d1
    swap    d1
    move.b  (a1)+,d2        ; \
    move.b  (a1)+,d3        ; /
    move.w  (a2,d2.w),d1
    move.b  (a2,d3.w),d1
    move.l  d1,(a0)+
    endm
.m2 macro
    move.b  (a1)+,d2        ; \
    move.b  (a1)+,d3        ; /
    move.w  (a2,d2.w),d1
    move.b  (a2,d3.w),d1
    move.w  d1,(a0)+
    endm
.m1 macro
    move.b  (a1)+,d2
    move.b  (a2,d2.w),(a0)+
    endm
.p4 macro
    move.b  1(a1),d2
    move.b  3(a1),d3
    addq.l  #4,a1
    move.b  (a2,d2.w),1(a0)
    addq.l  #4,a0
    move.b  (a2,d3.w),-1(a0)
    endm
.p2 macro
    move.b  1(a1),d2
    addq.l  #2,a1
    move.b  (a2,d2.w),1(a0)
    addq.l  #2,a0
    endm
.p1 macro
    inc_a0_a1
    endm
.q4 macro
    move.b  (a1),d2
    move.b  2(a1),d3
    addq.l  #4,a1
    move.b  (a2,d2.w),(a0)
    addq.l  #4,a0
    move.b  (a2,d3.w),-2(a0)
    endm
.q2 macro
    move.b  (a1),d2
    addq.l  #2,a1
    move.b  (a2,d2.l),(a0)
    addq.l  #2,a0
    endm
.q1 macro
    move.b  (a1)+,d2
    move.b  (a2,d2.l),(a0)+
    endm

    moveq   #0,d2
    moveq   #0,d3
    not.l   d1
    bne     .mask
    bin68k  .m4,.m2,.m1
.mask
    cmp.l   #$AAAAAAAA,d1
    bne     .l2
    bin68k  .p4,.p2,.p1
.l2
    cmp.l   #$55555555,d1
    bne     .l3
    bin68k  .q4,.q2,.q1
.l3
    msk68k  .m1,inc_a0_a1

*------------------------------------------------------------------------------------
    xdef    _setup
    
m68k_render
    dc.l    _RenderLine0
    REPT    14
    dc.l    _RenderLine2
    ENDR
    dc.l    _RenderLine1
    
ammx_render
    dc.l    _RenderLine0_AMMX
    REPT    14
    dc.l    _RenderLine2_AMMX
    ENDR
    dc.l    _RenderLine1_AMMX

* a3 = stack params ptr
* a4 = .epilogue
_setup
    movea.l (a3)+,a0    ; \ points to bottom
    movea.l (a3)+,a1    ; / fused
    cmpa.l  __ZN3dvl10gpBufStartE,a0
    bcc.b   .ok
    addq.l  #4,sp       ; tile largely above top of screen
    jmp     (a4)        ; just goto epilogue
.ok move.l  __ZN3dvl17light_table_indexE,d2
.table
    lea     m68k_render.l,a4
    movea.l (a3)+,a2    ; \ fused
    movea.l (a3)+,a3    ; /
    movea.l __ZN3dvl8gpBufEndE,a5
    addq.l  #4,a3       ; point to start
    move.l  (a4,d2.l*4),a4
.patch
    tst.b   _ac68080_ammx
    beq.b   .done
    move.l  #ammx_render,.table+2   ; change table for ammx
    move.w  #$4e75,.patch ; #rts
.done
    rts

prologue_7 macro
.size set 8
    movem.l d2-d5/a2-a5,-(sp)
    lea     (.size*4+4,sp),a3
    lea     .epilogue(pc),a4
    bsr     _setup
    endm
epilogue_7  macro
.epilogue
    move.l  (sp)+,d2        ; \ fused
    move.l  (sp)+,d3        ; /
    move.l  (sp)+,d4        ; \ fused
    move.l  (sp)+,d5        ; /
    move.l  (sp)+,a2        ; \ fused
    move.l  (sp)+,a3        ; /
    move.l  (sp)+,a4        ; \ fused
    move.l  (sp)+,a5        ; /
    rts
    endm

prologue_11 macro
.size set 11
    movem.l d2-d7/a2-a6,-(sp)
    lea     (.size*4+4,sp),a3
    lea     .epilogue(pc),a4
    bsr     _setup
    endm
epilogue_11 macro
.epilogue
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
    move.l  (sp)+,a6
    rts
    endm

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_TRANSPARENT(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)

_RenderTile_RT_TRANSPARENT
    prologue_11
    move.w  #32,a6              ; p1
    moveq   #0,d0               ; p2
.L1 move.l  -(a3),d6            ; p1
    subq.l  #1,a6               ; p2
    moveq   #32,d7              ; p1
    move.b  (a1)+,d0            ; p2
    bgt     .L4                 ; p1
    bra     .L3                 ; p2
.L2 move.l  d6,d1               ; p2   
    lsl.l   d0,d6               ; p1
    jsr     (a4)                ; p1 
    move.b  (a1)+,d0            ; p1
    bgt     .L4                 ; p1
.L3 add.b   d0,d7               ; p2
    beq     .L6                 ; p1
    neg.b   d0                  ; p2
    adda.l  d0,a0               ; p1
    lsl.l   d0,d6               ; p2
    move.b  (a1)+,d0            ; p1
    ble     .L3                 ; p1
.L4 sub.b   d0,d7               ; p2 
    bne     .L2                 ; p1
    move.l  d6,d1               ; p2   
    jsr     (a4)                ; p1
    sub.w   #BUFFER_WIDTH+32,a0 ; p1
    tst.l   a6                  ; p2
    bne     .L1                 ; p1
    epilogue_11
.L6 suba.w  #BUFFER_WIDTH+32-256,a0 ; p2
    tst.l   a6                  ; p1
    suba.w  d0,a0               ; p2
    bne     .L1                 ; p1
    bra     .epilogue           ; p2

_RenderTile_RT_TRANSPARENTorig
    prologue_11
    move.w  #32,a6          ; p1
    moveq   #0,d0           ; p2
.L1 move.l  -(a3),d6        ; p1 m = *mask; mask--
    subq.l  #1,a6           ; p2
    moveq   #32,d7          ; p1
.L2 move.b  (a1)+,d0        ; p2
    bgt.b   .L3             ; p1
.L22
    neg.b   d0              ; p2
    lsl.l   d0,d6           ; p1
    sub.l   d0,d7           ; p2
    adda.l  d0,a0           ; p1 doesnt affect the flags
    beq.b   .L5             ; p1 likely be false
    move.b  (a1)+,d0        ; p1
    ble.b   .L22            ; p1 more likely to be false at this point
.L3 move.l  d6,d1           ; p1
    lsl.l   d0,d6           ; p2
    sub.l   d0,d7           ; p1
    beq.b   .L4             ; p1
    jsr     (a4)            ; p1
    move.b  (a1)+,d0        ; p1
    bgt.b   .L3             ; p1
    bra     .L22            ; p2
.L4 jsr     (a4)            ; p1
.L5 tst.l   a6              ; p1
    sub.w   #BUFFER_WIDTH+32,a0   ; p2
    bne     .L1             ; p1
    epilogue_11

*------------------------------------------------------------------------------------

block16_ macro
    REPT    16
    moveq   #32,d0
    move.l  -(a3),d1
    jsr     (a4)
    sub.w   #BUFFER_WIDTH+32,a0
    ENDR
    endm

triangL_ macro
.i  set     30
    add.w   #.i,a0
    REPT    16
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    moveq   #32-.i,d0
    move.l  -(a3),d1
    jsr     (a4)
    IFNE    .i
.i  set     .i-2
    ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
    ENDR
    endm

triangR_ macro
.i  set     30
    REPT    16
    moveq   #32-.i,d0
    move.l  -(a3),d1
    jsr     (a4)
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
.i  set     .i-2
    ENDR
    endm
    
  ifeq  INLINE_BLOCK16
  
_block16
    block16_
    rts
_triangL
    triangL_
    rts
_triangR
    triangR_
    rts  
block16 macro
    bsr   _block16
    endm
triangL macro
    bsr   _triangL
    endm
triangR macro
    bsr   _triangR
    endm
  
  else
  
block16 macro
    block16_
    endm
triangL macro
    triangL_
    endm
triangR macro
    triangR_
    endm
    
  endc
  
*------------------------------------------------------------------------------------
* extern void RenderTile_RT_SQUARE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_SQUARE
    prologue_7
    block16
    block16
    epilogue_7

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_LTRAPEZOID(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_LTRAPEZOID
    prologue_7
    triangL
    block16
    epilogue_7

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_RTRAPEZOID(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_RTRAPEZOID
    prologue_7
    triangR
    block16
    epilogue_7

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_LTRIANGLE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_LTRIANGLE
    prologue_7
    triangL
.i  set     2
    addq.l  #.i,a0
    REPT    15
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    moveq   #32-.i,d0
    move.l  -(a3),d1
    jsr     (a4)
    IFNE    .i-30
.i  set     .i+2
    sub.w   #BUFFER_WIDTH+32-.i,a0
    ENDC
    ENDR
    epilogue_7

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_RTRIANGLE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_RTRIANGLE
    prologue_7
    triangR
.i  set     2
    REPT    15
    moveq   #32-.i,d0
    move.l  -(a3),d1
    jsr     (a4)
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
.i  set     .i+2
    ENDR
    epilogue_7

* end of file