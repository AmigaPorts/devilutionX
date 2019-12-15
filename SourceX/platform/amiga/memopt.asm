* -----------------------------------------------------------------------------
* memopt.asm -- AC68080 replacement for memxxx() operation with by hand-written
* asm code by S.Devulder
* -----------------------------------------------------------------------------

    section .text

    machine 68080

    XREF    _ac68080_ammx
    XREF    ___real_memcpy
    XDEF    ___wrap_memcpy
    XREF    ___real_memset
    XDEF    ___wrap_memset

    cnop    0,4

___wrap_memcpy
.entry
    tst.b   _ac68080_ammx
    beq.l   ___real_memcpy

.memcpy
    rsreset
      rs.l  4
.dst  rs.l  1
.src  rs.l  1
.len  rs.l  1

    move.l  .dst(sp),a1   ; p1 1
    move.l  .src(sp),a0   ; p1 2
    move.l  .len(sp),d1   ; p1 3

    moveq   #63,d0
    and.l   d1,d0
    lsr.l   #8,d1
    beq     .l32
.l64
    REPT    8
    load    (a0)+,e0
    store   e0,(a1)+
    ENDR
    subq.l  #1,d1
    bne.s   .l64
.l32
    bclr    #5,d0
    beq.b   .l16
    REPT    4
    load    (a0)+,e0
    store   e0,(a1)+
    ENDR
.l16
    load    (a0)+,e0
    storec   e0,d0,(a1)+
    subq.l  #8,d0
    bhi.b   .l16
    move.l  .dst(sp),d0
.exit
    nop
* remove initial comparison so that it now only costs 1 cycle
    move.w  #$203c,.entry(pc)   ; move.l #nnnn,d0
    move.w  #$223c,.entry+6(pc) ; move.l #nnnn,d1
    move.w  #$4e75,.exit(pc)    ; #rts
    rts                         ; no need to ClearCacheU on apollo!

___wrap_memset
.entry
    tst.b   _ac68080_ammx
    beq.l   ___real_memset

.memset
    rsreset
      rs.l  4
.dst  rs.l  1
.val  rs.l  1
.len  rs.l  1

    move.l  .dst(sp),a1   ; p1 1
    move.l  .val(sp),d0   ; p1 2
    move.l  .len(sp),d1   ; p1 3

    vperm   #$77777777,d0,e0,e0

    moveq   #63,d0
    and.l   d1,d0
    lsr.l   #8,d1
    beq     .l32
.l64
    REPT    8
    store   e0,(a1)+
    ENDR
    subq.l  #1,d1
    bne.s   .l64
.l32
    bclr    #5,d0
    beq.b   .l16
    REPT    4
    store   e0,(a1)+
    ENDR
.l16
    storec   e0,d0,(a1)+
    subq.l  #8,d0
    bhi.b   .l16
    move.l  .dst(sp),d0
.exit
    nop
* remove initial comparison so that it now only costs 1 cycle
    move.w  #$203c,.entry(pc)   ; move.l #nnnn,d0
    move.w  #$223c,.entry+6(pc) ; move.l #nnnn,d1
    move.w  #$4e75,.exit(pc)    ; #rts
    rts                         ; no need to ClearCacheU on apollo!

* end of file