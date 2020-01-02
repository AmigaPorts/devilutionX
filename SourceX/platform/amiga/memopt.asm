* -----------------------------------------------------------------------------
* memopt.asm -- AC68080 replacement for memxxx() operation with by hand-written
* asm code by S.Devulder
* -----------------------------------------------------------------------------
    
    section .text

    machine 68080

    XREF    ___real_memcpy
    XDEF    ___wrap_memcpy
    
    XREF    ___real_memset
    XDEF    ___wrap_memset
    
*   XREF    ___real_memcmp
    XDEF    ___wrap_memcmp
    
    XDEF    _ConvertUInt16BufferAMMX
    XDEF    _ConvertUInt32BufferAMMX
    XDEF    _ConvertUInt64BufferAMMX
    XREF    _ac68080_ammx

    cnop    0,4
    
memcpy_68k
    jmp     ___real_memcpy

___wrap_memcpy

      rsreset
      rs.l  1
.dst  rs.l  1
.src  rs.l  1
.len  rs.l  1

*    bra   memcpy_68k

.entry
    tst.b   _ac68080_ammx
    beq.b   memcpy_68k

    move.l  .dst(sp),d0   ; p1 1
    move.l  .src(sp),a0   ; p1 2
    move.l  .len(sp),d1   ; p1 3    
    movea.l d0,a1
    
.loop
    load    (a0)+,e0
    storec  e0,d1,(a1)+
    subq.l  #8,d1
    bhi.s   .loop

.exit
* remove initial comparison so that it now only costs 1 cycle
    move.w  #$203c,.entry       ; move.l #nnnn,d0
    move.w  #$7200,.entry+6     ; moveq.w #0,d1
    move.w  #$4e75,.exit        ; #rts
    rts                         ; no need to ClearCacheU on apollo!

memset_68k
    jmp     ___real_memset
    
___wrap_memset
      rsreset
      rs.l  1
.dst  rs.l  1
.val  rs.l  1
.len  rs.l  1

.entry
    tst.b   _ac68080_ammx
    beq.b   memset_68k

    move.l  .dst(sp),a0  ; p1 1
    move.l  .val(sp),d0   ; p1 2
    move.l  .len(sp),d1   ; p1 3

    vperm   #$77777777,d0,e0,e0
    move.l  a0,d0

.loop
    storec   e0,d1,(a0)+
    subq.l  #8,d1
    bhi.b   .loop

.exit
* remove initial comparison so that it now only costs 1 cycle
    move.w  #$203c,.entry       ; move.l #nnnn,d0
    move.w  #$7200,.entry+6     ; moveq  #0,d1
    move.w  #$4e75,.exit        ; #rts
    rts                         ; no need to ClearCacheU on apollo!

___wrap_memcmp
    rsreset
      rs.l  1
.sc1  rs.l  1
.sc2  rs.l  1
.len  rs.l  1

    move.l  .sc1(sp),a0
    move.l  .sc2(sp),a1
    move.l  .len(sp),d0
    
.l0
    subq.l  #8,d0
    bcs     .l1
    cmp.l   (a1)+,(a0)+
    bne     .ne
    cmp.l   (a1)+,(a0)+
    beq     .l0
.ne
    bcs     .lt
    moveq   #1,d0
    rts
.lt
    moveq   #-1,d0
    rts
.l1
    bclr    #2,d0
    beq     .l2
    cmp.l   (a1)+,(a0)+
    bne     .ne
.l2
    bclr    #1,d0
    beq     .l3
    cmp.w   (a1)+,(a0)+
    bne     .ne
.l3
    addq.l  #8,d0
    beq     .eq
    cmp.b   (a1)+,(a0)+
    bne     .ne
.eq
    moveq   #0,d0
    rts
    
_ConvertUInt16BufferAMMX
      rsreset
      rs.l  1
.ptr  rs.l  1
.len  rs.l  1

    move.l  .ptr(sp),a0
    move.l  .len(sp),d0
    and.w   #-2,d0
.loop
    load    (a0),d1
    vperm   #$10325476,d1,d1,d1
    storec  d1,d0,(a0)+
    subq.l  #8,d0
    bhi     .loop
    rts
    

_ConvertUInt32BufferAMMX
      rsreset
      rs.l  1
.ptr  rs.l  1
.len  rs.l  1

    move.l  .ptr(sp),a0
    move.l  .len(sp),d0
    and.w   #-4,d0

    movem.l d2/d3,-(sp)    
    moveq   #64,d1
    sub.l   d1,d0
    movea.l  a0,a1
    bcs     .l32
.l64
    REPT    8
    movex.l (a0)+,d2
    movex.l (a0)+,d3
    move.l  d2,(a1)+
    move.l  d3,(a1)+
    ENDR
    sub.l   d1,d0
    bcc     .l64
.l32
    add.l   d1,d0
    beq     .exit2
    bclr    #5,d0
    beq.b   .l16
    REPT    4
    movex.l (a0)+,d2
    movex.l (a0)+,d3
    move.l  d2,(a1)+
    move.l  d3,(a1)+
    ENDR
.l16
    bclr    #4,d0
    beq.b   .l8
    REPT    2
    movex.l (a0)+,d2
    movex.l (a0)+,d3
    move.l  d2,(a1)+
    move.l  d3,(a1)+
    ENDR
.l8
    load    (a0)+,d2
    vperm   #$32107654,d2,d2,d2
    storec  d2,d0,(a1)+
    subq.l  #8,d0
    bls.b   .exit2
    load    (a0)+,d2
    vperm   #$32107654,d2,d2,d2
    storec  d2,d0,(a1)+
.exit2
    movem.l (sp)+,d2/d3
    rts
    
_ConvertUInt64BufferAMMX
      rsreset
      rs.l  1
.ptr  rs.l  1
.len  rs.l  1

    move.l  .ptr(sp),a0
    move.l  .len(sp),d0
    and.w   #-8,d0
.loop
    load    (a0),d1
    vperm   #$76543210,d1,d1,d1
    storec  d1,d0,(a0)+
    subq.l  #8,d0
    bhi     .loop
    rts
    
* end of file