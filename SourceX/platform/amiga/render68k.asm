* -----------------------------------------------------------------------------
* render68k.asm -- replacement of C code by hand-written asm code by S.Devulder
* -----------------------------------------------------------------------------
    machine 68080

    section .text

BUFFER_WIDTH    set     768
VAMP_V4         set     1   ; 0 = replaces movem with separate moves
NO_OVERDRAW     set     1   ; 1 = tests for out of screen drawings (0=crash)
A5_RELATIVE     set     1   ; 1 = faster out of bounds tests (AMMX)
USE_CMP2        set     0   ; 1 = uses CMP2 (might be faster on 68K)
USE_BANK        set     1   ; uses E4/E5 in place of D4/D5 in Render2_AMMX

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

  ifne  NO_OVERDRAW
    XREF    __ZN3dvl10gpBufStartE
  ifeq  USE_CMP2
    XREF    __ZN3dvl8gpBufEndE
  endc
  endc

    XREF    __ZN3dvl8lightmaxE
    XREF    __ZN3dvl17light_table_indexE
    XREF    _ac68080_ammx
    
    cnop    0,4

* sanity
  ifeq  NO_OVERDRAW
A5_RELATIVE set 0
USE_CMP2    set 0
  endc

  ifne  A5_RELATIVE
USE_CMP2    set 0
  endc

  ifeq    NO_OVERDRAW*(1-USE_CMP2)
SAVE_A5A6   set 0
  else
SAVE_A5A6   set 1
  endc


bank macro
  ifne  USE_BANK
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
  else
    ifb   \5
      \3    \4
    else
      \3    \4,\5
    endc
  endc
  endm

* -----------------------------------------------------------------------------
* check bounds

rts_bounds macro
    ifnb  \1
      adda.w  d0,\1
    endc
    ifnb  \2
      adda.w  d0,\2
    endc
    ifne  A5_RELATIVE
      suba.l  a5,a0
    endc
    rts
  endm

chk_bounds macro
*  beq.b   \1     ; 1
  ifne  NO_OVERDRAW
    ifne  A5_RELATIVE
      cmp.l   a6,a0  ; 1
      adda.l  a5,a0  ; 1
      bhi.b   \1     ; 2
    else
      ifne  USE_CMP2
        cmp2.l  __ZN3dvl10gpBufStartE,a0
        bcs.b   \1
      else
        cmp.l   a5,a0  ; 1
        bcs.b   \1     ; 2
        cmp.l   a6,a0  ; 3
        bhi.b   \1     ; 4
      endc
*   move.l  a0,d2   ; F p1
*   sub.l   a5,d2   ; F p1
*   move.l  a6,d3   ; F p2
*   sub.l   a5,d3   ; F p2 = 1 cycles for all 4 isntructions
*   cmp.l   d3,d2   ; 2
*   bhi     \1      ; 3 total cycles
      endc
    endc
  endm

* -----------------------------------------------------------------------------
* debug: displays nothing

_RenderLine_NONE
    move.l  -(a3),d1
    add.w   d0,a1
    add.w   d0,a0
    rts

* -----------------------------------------------------------------------------
* AMMX version

rol_d1_mask macro
    bfclr   d1{d0:8}
    rol.l   #8,d1
**  move.w  #$ff00,d3
*   lsr.w   d0,d3
*   rol.l   #8,d1
*   and.w   d3,d1
    endm

unroll_AMMX macro
    bclr    #5,d0
    beq     \1_16
* 32 bytes in a row
    \1      0
    \1      0
    \1      0
    \1      1
    rts_bounds
\1_16
    bclr    #4,d0
    beq     \1_8
* 16 bytes in a row
    \1      0
    \1      0
\1_8
    bclr    #3,d0
    beq     \1_0
* 8 bytes in a row
    \1      0
\1_0
* 0 to 7 bytes
    \2
* fixup ptrs
\3
    endm

* case light_table_index == lightmax
_RenderLine1_AMMX
    move.l  -(a3),d1
    add.w d0,a1
    chk_bounds  .nx

    peor    d2,d2,d2    ; d2=0.q

    moveq   #1,d3
    add.l   d1,d3
    bne     .mask

* no mask
.n8 macro
    store   d2,(a0)+
    endm
.n0 macro
    storec  d2,d0,(a0)
    endm
    unroll_AMMX  .n8,.n0,.nx
    add.w d0,a0
    rts_bounds

* mask version
.m8 macro
    rol.l   #8,d1
    storem  d2,d1,(a0)+
    endm
.m0 macro
    rol_d1_mask
    storem  d2,d1,(a0)
    endm
.mask
    unroll_AMMX  .m8,.m0,.mx
    add.w   d0,a0
    rts_bounds

* case light_table_index == 0
_RenderLine0_AMMX
    move.l  -(a3),d1
    chk_bounds  .nx

    moveq   #1,d3
    add.l   d1,d3
    bne     .mask

* no mask
.n8 macro
    load    (a1)+,d2
    store   d2,(a0)+
    endm
.n0 macro
    load    (a1),d2
    storec  d2,d0,(a0)
    endm
    unroll_AMMX  .n8,.n0,.nx
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds

* version with mask
.m8 macro
    rol.l   #8,d1
    load    (a1)+,d2
    storem  d2,d1,(a0)+
    endm
.m0 macro
    rol_d1_mask
    load    (a1),d2
    storem  d2,d1,(a0)
    endm
.mask
    unroll_AMMX  .m8,.m0,.mx
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds

* other cases
transform   macro
*   move.l  (a1)+,d3        ; F(used)  d3=AABBCCDD
*   move.l  (a1)+,d5        ; F  1
* input d3/d5
  ifne  \1&$a0
    move.l  d3,d2           ; p1      d3=AABBCCDD
    rol.l   #8,d2           ; p1      d2=BBCCDDAA
  endc
  ifne  \1&$0a
    bank  1,1,move.l,d5,d4           ; p2
    bank  0,1,rol.l,#8,d4            ; p2 2
  endc
  ifne  \1&$a0
    and.l #$00FF00FF,d2   ; p1      d2=00CC00AA
  endc
  ifne  \1&$50
    and.l #$00FF00FF,d3   ; p2 3    d3=00BB00DD
  endc
  ifne  \1&$0a
    bank  0,1,and.l,#$00FF00FF,d4   ; p1
  endc
  ifne  \1&$05
    bank  0,1,and.l,#$00FF00FF,d5   ; p2 4
  endc
  ifne  \1&$50
    swap  d3              ; p1      d3=00DD00BB
  endc
  ifne  \1&$05
    bank  0,1,swap,d5              ; p2 5
  endc
  ifne  \1&$80
    move.w  (a2,d2.w),d2    ; p1 6    d2=00CCxx--
  endc
  ifne    \1&$40
    move.b  (a2,d3.w),d2    ; p1      d2=00CCxxyy
  endc
  ifne  \1&$a0
    swap  d2              ; p2 7    d2=xxyy00CC
  endc
  ifne  \1&$50
    swap  d3              ; p1      d3=00BB00DD
  endc
  ifne  \1&$08
    bank  1,1,move.w,(a2,d4.w),d4    ; p2 8
  endc
  ifne  \1&$04
    bank  1,1,move.b,(a2,d5.w),d4    ; p1
  endc
  ifne  \1&$0a
    bank  0,1,swap,d4              ; p2 9
  endc
  ifne  \1&$05
    bank  0,1,swap,d5              ; p1
  endc
  ifne    \1&$20
    move.w  (a2,d2.w),d2    ; p2 10   d2=xxyyzz--
  endc
  ifne    \1&$10
    move.b  (a2,d3.w),d2    ; p1 11   d2=xxyyzztt
  endc
  ifne    \1&$02
    bank  1,1,move.w,(a2,d4.w),d4    ; p1 12
  endc
  ifne    \1&$01
    bank  1,1,move.b,(a2,d5.w),d4    ; p1 13
  endc
* output d2/d4
*   move.l  d2,(a0)+        ; F
*   move.l  d4,(a0)+        ; F  14 ==> 14 cycles for 8 pixels ?
  endm

push_d4_d5  macro
  ifeq  USE_BANK
    movem.l d4/d5,-(sp)
  endc
  endm
pull_d4_d5  macro
  ifeq  USE_BANK
   ifne  VAMP_V4
    movem.l (sp)+,d4/d5
   else
    move.l  (sp)+,d4
    move.l  (sp)+,d5
   endc
  endc
  endm

_RenderLine2_AMMX
    move.l  -(a3),d1
    chk_bounds  _RenderLine0_AMMX\.mx

    push_d4_d5

    move.l  d1,d3               ; \ fused
    addq.l  #1,d3               ; /
    bne     .mask

.n8 macro
    move.l  (a1)+,d3
    bank  0,1,move.l,(a1)+,d5
    transform   $ff
    move.l  d2,(a0)+
    bank  1,0,move.l,d4,(a0)+
    ifne    \1
      pull_d4_d5
    endc
  endm
.n0 macro
    move.l  (a1),d3
    bank    0,1,move.l,4(a1),d5
    transform   $ff
    ifne USE_BANK
    vperm   #$4567CDEF,d2,e4,d2
    else
    vperm   #$4567CDEF,d2,d4,d2
    endc
    storec  d2,d0,(a0)
  endm
    unroll_AMMX  .n8,.n0,.nx
    pull_d4_d5
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds

* mask versions

.transfAA55_8 macro
    move.b  \1(a1),d2               ; 1
    move.b  \1+2(a1),d3             ; 2
    bank    0,1,move.b,\1+4(a1),d4  ; 3
    bank    0,1,move.b,\1+6(a1),d5  ; 4
    addq.l  #8,a0                   ; 4
    addq.l  #8,a1                   ; 5
    move.w  (a2,d2.w),d1            ; 5
    move.b  (a2,d3.w),d1            ; 6
    swap    d1                      ; 7
    bank    1,0,move.w,(a2,d4.w),d1 ; 8
    bank    1,0,move.b,(a2,d5.w),d1 ; 9
    movep.l d1,\1-8(a0)             ; 10
  endm

.transfAA55 macro
    moveq   #0,d2
    moveq   #0,d3
    bank    0,1,moveq,#0,d4
    bank    0,1,moveq,#0,d5
    
    bclr    #5,d0
    beq     .b4\2
    .transfAA55_8 \1
    .transfAA55_8 \1
    .transfAA55_8 \1
    .transfAA55_8 \1
    pull_d4_d5
    rts_bounds
.b4\2
    bclr    #4,d0
    beq.b   .b3\2
    .transfAA55_8 \1
    .transfAA55_8 \1
.b3\2
    bclr    #3,d0
    beq.b   .b2\2
    .transfAA55_8 \1
.b2\2
    pull_d4_d5
    bclr    #2,d0
    beq.b   .b1\2
    move.b  \1(a1),d2     ; 1
    move.b  \1+2(a1),d3   ; 2
    addq.l  #4,a0         ; 2
    addq.l  #4,a1         ; 3
    move.w  (a2,d2.w),d1  ; 3+1
    move.b  (a2,d3.w),d1  ; 5
    movep.w d1,\1-4(a0)   ; 6
.b1\2
    bclr    #1,d0
    beq.b   .b0\2
    move.b  \1(a1),d2     ; 1
    addq.l  #2,a0
    addq.l  #2,a1         ; 2
; 2 bubbles
    move.b  (a2,d2.w),d1  ; 4
    move.b  d1,\1-2(a0)   ; 5
.b0\2
  ifeq  \1
    tst.b   d0
    beq.b   .bb0\2
    move.b  (a1)+,(a0)+
.bb0\2
  else
    add.w   d0,a0
    add.w   d0,a1
  endc
    rts_bounds
  endm
  
.maskAA
    .transfAA55   0,_aa

.mask55
    .transfAA55   1,_55


.mask
    move.l  #$AAAAAAAA,d3
    eor.l   d1,d3
    beq     .maskAA    
    not.l   d3
    beq     .mask55

.m8 macro
    move.l  (a1)+,d3        ; F(used)  d3=AABBCCDD
    bank    0,1,move.l,(a1)+,d5        ; F  1
    rol.l   #8,d1
    transform   $ff
    ifne USE_BANK
    vperm   #$4567CDEF,d2,e4,d2
    else
    vperm   #$4567CDEF,d2,d4,d2
    endc
    storem  d2,d1,(a0)+
    ifne    \1
    pull_d4_d5
    endc
    endm
.m0 macro
    move.l  (a1),d3         ; F(used)  d3=AABBCCDD
    bank    0,1,move.l,4(a1),d5        ; F  1
    rol_d1_mask
    transform   $ff
    ifne USE_BANK
    vperm   #$4567CDEF,d2,e4,d2
    else
    vperm   #$4567CDEF,d2,d4,d2
    endc
    storem  d2,d1,(a0)
    endm

.maskXX
    unroll_AMMX  .m8,.m0,.mx
    pull_d4_d5
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds
    

* -----------------------------------------------------------------------------

_RenderLine2_AMMX_orig
    move.l  -(a3),d1
    chk_bounds  _RenderLine2_AMMX\.mx

    move.l  d1,d3               ; \ fused
    addq.l  #1,d3               ; /
    bne     .mask

* here d3=0 => no need to init

transform   macro
    IFNE    \1-$AA
    vperm   #$A7A5A3A1,d2,d3,d3
    ENDC
    IFNE    \1-$55
    vperm   #$A6A4A2A0,d2,d3,d2
    ENDC

    IFNE    \1&$80
    move.w  (a2,d2.w),d2
    ENDC
    IFNE    \1&$40
    move.b  (a2,d3.w),d2
    ENDC

    swap    d2
    swap    d3

    IFNE    \1&$20
    move.w  (a2,d2.w),d2
    ENDC
    IFNE    \1&$10
    move.b  (a2,d3.w),d2
    ENDC

    vperm   #$45670123,d2,d2,d2
    IFNE    \1-$AA
*   lsrq    #32,d3              ; doesn't compile with vasm
    vperm   #$00000123,d3,d3,d3
    ENDC

    IFNE    \1&$8
    move.w  (a2,d2.w),d2
    ENDC
    IFNE    \1&$4
    move.b  (a2,d3.w),d2
    ENDC

    swap    d2
    swap    d3

    IFNE    \1&$2
    move.w  (a2,d2.w),d2
    ENDC
    IFNE    \1&$1
    move.b  (a2,d3.w),d2
    ENDC
    endm

.n8 macro
    load    (a1)+,d2
    transform   $ff
    store   d2,(a0)+
    endm
.n0 macro
    load    (a1),d2
    transform   $ff
    storec  d2,d0,(a0)
    endm
    unroll_AMMX  .n8,.n0,.nx
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds

* mask version
.mask
    moveq   #0,d3

    cmp.l   #$AAAAAAAA,d1
    beq     .maskAA
    cmp.l   #$55555555,d1
    beq     .mask55

.m8 macro
    load    (a1)+,d2
    transform   $ff
    rol.l   #8,d1
    storem  d2,d1,(a0)+
    endm
.m0 macro
    load    (a1),d2
    transform   $ff
    rol_d1_mask
    storem  d2,d1,(a0)
    endm
    unroll_AMMX  .m8,.m0,.mx
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds

.m8AA   macro
    load    (a1)+,d2
    transform   $AA
    rol.l   #8,d1
    storem  d2,d1,(a0)+
    endm
.m0AA   macro
    load    (a1),d2
    transform   $AA
    rol_d1_mask
    storem  d2,d1,(a0)
    endm
.maskAA
    unroll_AMMX  .m8AA,.m0AA,.mxAA
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds

.m855   macro
    load    (a1)+,d2
    transform   $55
    rol.l   #8,d1
    storem  d2,d1,(a0)+
    endm
.m055   macro
    load    (a1),d2
    transform   $55
    rol_d1_mask
    storem  d2,d1,(a0)
    endm
.mask55
    unroll_AMMX  .m855,.m055,.mx55
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds

* -----------------------------------------------------------------------------

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
    addq.l  #1,a0
    addq.l  #1,a1
    endm

unroll macro
    bclr  #5,d0
    beq   \1_4
    \1    0
    \1    0
    \1    0
    \1    0
    \1    0
    \1    0
    \1    0
    \1    1
    rts_bounds
\1_4
    bclr  #4,d0
    beq   \1_2
    \1    0
    \1    0
    \1    0
    \1    0
\1_2
    bclr  #3,d0
    beq   \1_1
    \1    0
    \1    0
\1_1
    bclr  #2,d0
    beq   \2_1
    \1    0
\2_1
    bclr #1,d0
    beq  \3_1
    \2
\3_1
    tst.b d0
    beq   \3_2
    \3
\3_2
    rts_bounds
    endm

loop macro
\1__1
    add.l   d1,d1
    bcs     \1__2
    \1
    subq.w  #1,d0
    bne     \1__1
    rts_bounds
\1__2
    \2
    subq.w  #1,d0
    bne     \1__1
    rts_bounds
    endm

* case light_table_index == 0
_RenderLine0_
    add.w   d0,a0
    add.w   d0,a1
    rts_bounds
_RenderLine0
    move.l  -(a3),d1
    chk_bounds  _RenderLine0_

    not.l   d1
    bne     .mask
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

.mask
    cmp.l   #$AAAAAAAA,d1           ; bg / fg / bg fg
    bne     .l2
.p4 macro
    ifeq 1
    move.b  1(a1),1(a0)             ; 2
    move.b  3(a1),3(a0)             ; 2
    addq.l  #4,a0                   ; .5 ==> 5 cycles
    addq.l  #4,a1                   ; .5
    else
    move.l  (a1)+,d1                ; \
    and.l   #$00FF00FF,d1           ; / 1 cycle (fused) ?
    move.l  (a0),d2                 ; \
    and.l   #$FF00FF00,d2           ; / 1 cycle (fused) ?
    or.l    d2,d1                   ; 1
    move.l  d1,(a0)+                ; 1 ==> 4 cycles
    endc
    endm
.p2 macro
    move.b  1(a1),1(a0)
    addq.l  #2,a0
    addq.l  #2,a1
    endm
.p1 macro
    addq.l  #1,a0
    addq.l  #1,a1
    endm
    unroll  .p4,.p2,.p1

.l2
    cmp.l   #$55555555,d1           ; fg / bg /fg / bg
    bne     .l3

.q4 macro
    ifeq 1
    move.b  (a1),(a0)               ; 2
    move.b  2(a1),2(a0)             ; 2
    addq.l  #4,a1                   ; .5
    addq.l  #4,a0                   ; .5 ==> 5 cycles
    else
    move.l  #$FF00FF00,d1
    and.l   (a1)+,d1
    move.l  #$00FF00FF,d2
    and.l   (a0),d2
    or.l    d2,d1
    move.l  d1,(a0)+
    endc
    endm
.q2 macro
    ifeq 1
    move.b  (a1),(a0)
    addq.l  #2,a1
    addq.l  #2,a0
    else
    move.w  #$FF00,d1
    and.w   (a1)+,d1
    move.w  #$00FF,d2
    and.w   (a0),d2
    or.w    d2,d1
    move.w  d1,(a0)+
    endc
    endm
.q1 macro
    move.b  (a1)+,(a0)+
    endm
    unroll  .q4,.q2,.q1

.l3
    loop    .m1,inc_a0_a1


* case light_table_index == lightmax
_RenderLine1_
    add.w   d0,a0
    rts_bounds
_RenderLine1
    move.l  -(a3),d1
    add.w   d0,a1
    chk_bounds _RenderLine1_

    not.l   d1
    bne     .mask

.m4 macro
    clr.l   (a0)+
    endm
.m2 macro
    clr.w   (a0)+
    endm
.m1 macro
    clr.b   (a0)+
    endm
    unroll  .m4,.m2,.m1
.mask
    cmp.l   #$AAAAAAAA,d1
    bne     .l2
    move.l  #$FF00FF00,d2
.p4 macro
    and.l   d2,(a0)+
    endm
.p2 macro
    and.w   d2,(a0)+
    endm
.p1 macro
    addq.l  #1,a0
    endm
    unroll  .p4,.p2,.p1

.l2
    cmp.l   #$55555555,d1
    bne     .l3
    move.l  #$00FF00FF,d2
.q4 macro
    and.l   d2,(a0)+
    endm
.q2 macro
    and.w   d2,(a0)+
    endm
.q1 macro
    clr.b   (a0)+
    endm
    unroll  .q4,.q2,.q1
.l3
    loop    .m1,inc_a0

* other cases
_RenderLine2_
    add.w   d0,a1
    add.w   d0,a0
    rts_bounds
_RenderLine2
    move.l  -(a3),d1
    chk_bounds  _RenderLine2_

    moveq   #0,d2
    moveq   #0,d3

    not.l   d1
    bne     .mask
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
    unroll  .m4,.m2,.m1

.mask
    cmp.l   #$AAAAAAAA,d1
    bne     .l2
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
    unroll  .p4,.p2,.p1

.l2
    cmp.l   #$55555555,d1
    bne     .l3
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
    unroll  .q4,.q2,.q1

.l3
    loop    .m1,inc_a0_a1

*------------------------------------------------------------------------------------
setup macro
* get params from stack
    IFNE    VAMP_V4
    movem.l (4*(1+\1),sp),a0/a1/a2/a3
    ELSE
    move.l  (4*(1+\1),sp),a0    ; \
    move.l  (4*(2+\1),sp),a1    ; / fused

    move.l  (4*(3+\1),sp),a2    ; \ fused
    move.l  (4*(4+\1),sp),a3    ; /
    ENDC
    ifne    USE_BANK
    inline
    tst.b   _ac68080_ammx
    beq   .1
    bank  0,1,move.l,a2,a2
.1
    einline
    endc
    bsr     _setup
    endm

  xdef  _setup

_setup
  ifne  NO_OVERDRAW
   ifeq  USE_CMP2
    move.l  __ZN3dvl10gpBufStartE,a5
    move.l  __ZN3dvl8gpBufEndE,a6
   endc
   ifne  A5_RELATIVE
    sub.l   a5,a0
    sub.l   a5,a6
   endc
  endc
    addq.l  #4,a3
* determine renderFcn
.ammx
    tst.b   _ac68080_ammx
    beq.b   .m68k

    lea     _RenderLine0_AMMX(pc),a4
    move.l  __ZN3dvl17light_table_indexE,d2
    beq.b   .exit
    sub.b   __ZN3dvl8lightmaxE,d2
    lea     _RenderLine2_AMMX(pc),a4
    bne.b   .exit
    lea     _RenderLine1_AMMX(pc),a4
.exit
* remove initial comparison so that it now only costs 1 cycle
    move.w  #$203c,.ammx       ; move.l #nnnn,d0
    move.w  #$7200,.ammx+6     ; moveq  #0,d1
    move.w  #$4e75,.exit       ; #rts
    rts                        ; no need to ClearCacheU on apollo!

.m68k
    lea     _RenderLine0(pc),a4
    move.l  __ZN3dvl17light_table_indexE,d2
    beq     .L0
    sub.b   __ZN3dvl8lightmaxE,d2
    lea     _RenderLine2(pc),a4
    bne.b   .L0
    lea     _RenderLine1(pc),a4
.L0
    rts
    
prologue_7 macro
.size set 7
    ifeq SAVE_A5A6
.size set .size-2
    endc
    IFNE    VAMP_V4
    ifne  SAVE_A5A6
    movem.l d2-d3/a2-a6,-(sp)
    else
    movem.l d2-d3/a2-a4,-(sp)
    endc
    ELSE
    sub.w   #4*.size,sp
    move.l  d2,4*0(sp)      ; \
    move.l  d3,4*1(sp)      ; / fused

    move.l  a2,4*2(sp)      ; \
    move.l  a3,4*3(sp)      ; / fused

    move.l  a4,4*4(sp)      ; \
    ifne    SAVE_A5A6
    move.l  a5,4*5(sp)      ; / fused

    move.l  a6,4*6(sp)
    endc
    ENDC
    setup   .size
    endm

epilogue_7  macro
    IFNE    VAMP_V4
    ifne    SAVE_A5A6
    movem.l (sp)+,d2-d3/a2-a3/a4-a6
    else
    movem.l (sp)+,d2-d3/a2-a3/a4
    endc
    ELSE
    move.l  (sp)+,d2        ; \ fused
    move.l  (sp)+,d3        ; /
    move.l  (sp)+,a2        ; \ fused
    move.l  (sp)+,a3        ; /
    move.l  (sp)+,a4        ; \ fused
    ifne    SAVE_A5A6
    move.l  (sp)+,a5        ; /
    move.l  (sp)+,a6
    endc
    ENDC
    rts
    endm

prologue_11 macro
.size set 11
    ifeq    SAVE_A5A6
.size set .size-2
    endc
    IFNE    VAMP_V4
    ifne    SAVE_A5A6
    movem.l d2-d7/a2-a6,-(sp)
    else
    movem.l d2-d7/a2-a4,-(sp)
    endc
    ELSE
    sub.w   #4*.size,sp

    move.l  d2,4*0(sp)      ; \
    move.l  d3,4*1(sp)      ; / fused

    move.l  d4,4*2(sp)      ; \
    move.l  d5,4*3(sp)      ; / fused

    move.l  d6,4*4(sp)      ; \
    move.l  d7,4*5(sp)      ; / fused

    move.l  a2,4*6(sp)      ; \
    move.l  a3,4*7(sp)      ; / fused

    move.l  a4,4*8(sp)       ; \
    ifne    SAVE_A5A6
    move.l  a5,4*9(sp)      ; / fused

    move.l  a6,4*10(sp)
    endc
    ENDC
    setup   .size
    endm

epilogue_11 macro
    IFNE    VAMP_V4
    ifne    SAVE_A5A6
    movem.l (sp)+,d2-d7/a2-a6
    else
    movem.l (sp)+,d2-d7/a2-a4
    endc
    ELSE
    move.l  (sp)+,d2        ; \ fused
    move.l  (sp)+,d3        ; /
    move.l  (sp)+,d4        ; \ fused
    move.l  (sp)+,d5        ; /
    move.l  (sp)+,d6        ; \ fused
    move.l  (sp)+,d7        ; /
    move.l  (sp)+,a2        ; \ fused
    move.l  (sp)+,a3        ; /
    move.l  (sp)+,a4        ; \ fused
    ifne    SAVE_A5A6
    move.l  (sp)+,a5        ; /
    move.l  (sp)+,a6
    endc
    ENDC
    rts
    endm

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_TRANSPARENT(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_TRANSPARENT
    prologue_11
    addq.l  #2,a4           ; skip over load mask
    moveq   #31,d7
.L1
    move.l  -(a3),d6        ; m = *mask; mask--
    moveq   #32,d4
.L2
    moveq   #0,d0           ; TODO: remove ?
    move.b  (a1)+,d0
    bgt.b   .L3
.L22
    neg.b   d0              ; p1
    lsl.l   d0,d6           ; p1
    sub.l   d0,d4           ; p2
    adda.l  d0,a0           ; p1 doesnt affect the flags
    beq.b   .L5             ; p2 likely be false
    move.b  (a1)+,d0        ; p1
    ble.b   .L22            ; p1 more likely to be false at this point
.L3
    move.l  d6,d1
    lsl.l   d0,d6
    sub.l   d0,d4
    beq     .L4             ; likely to be false most of the times
    jsr     (a4)
    moveq   #0,d0           ; TODO: remove ?
    move.b  (a1)+,d0
    ble.b   .L22            ; more likely at this point
    bra     .L3
.L4
    jsr     (a4)
.L5
    sub.w   #BUFFER_WIDTH+32,a0
    dbra    d7,.L1
    epilogue_11

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_SQUARE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_SQUARE
    prologue_7
    bsr     block16
    bsr     block16
    epilogue_7

*------------------------------------------------------------------------------------

    XDEF    block16
    XDEF    triangL
    XDEF    triangR

block16
    REPT    16
    moveq   #32,d0
    jsr     (a4)
    sub.w   #BUFFER_WIDTH+32,a0
    ENDR
    rts

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_LTRAPEZOID(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_LTRAPEZOID
    prologue_7
    bsr     triangL
    bsr     block16
    epilogue_7

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_RTRAPEZOID(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_RTRAPEZOID
    prologue_7
    bsr     triangR
    bsr     block16
    epilogue_7

triangL
.i  set     30
    add.w   #.i,a0
    REPT    16
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    moveq   #32-.i,d0
    jsr     (a4)
    IFNE    .i
.i  set     .i-2
    ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
    ENDR
    rts

triangR
.i  set     30
    REPT    16
    moveq   #32-.i,d0
    jsr     (a4)
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
.i  set     .i-2
    ENDR
    rts

*------------------------------------------------------------------------------------
* extern void RenderTile_RT_LTRIANGLE(BYTE *dst, BYTE *src, BYTE *tbl, DWORD *mask)
_RenderTile_RT_LTRIANGLE
    prologue_7
    bsr     triangL
.i  set     2
    addq.l  #.i,a0
    REPT    15
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    moveq   #32-.i,d0
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
    bsr     triangR
.i  set     2
    REPT    15
    moveq   #32-.i,d0
    jsr     (a4)
    IFNE    .i&2
    addq.w  #2,a1
    ENDC
    sub.w   #BUFFER_WIDTH+32-.i,a0
.i  set     .i+2
    ENDR
    epilogue_7

* end of file