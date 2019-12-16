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
    
*   XREF    ___real_memcmp
    XDEF    ___wrap_memcmp

    cnop    0,4

___wrap_memcpy
.entry
    tst.b   _ac68080_ammx
    beq.l   ___real_memcpy

.memcpy
    rsreset
      rs.l  1
.dst  rs.l  1
.src  rs.l  1
.len  rs.l  1

    move.l  .dst(sp),a1   ; p1 1
    move.l  .src(sp),a0   ; p1 2
    move.l  .len(sp),d0   ; p1 3

    moveq   #64,d1
    sub.l   d1,d0
    bcs     .l32
.l64
    REPT    8
    load    (a0)+,e0
    store   e0,(a1)+
    ENDR
    sub.l   d1,d0
    bcc     .l64
.l32
    add.l   d1,d0
    beq     .exit2
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
    bcs.b   .exit2
    load    (a0)+,e0
    storec   e0,d0,(a1)+
.exit2
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
      rs.l  1
.dst  rs.l  1
.val  rs.l  1
.len  rs.l  1

    move.l  .dst(sp),a1   ; p1 1
    move.l  .val(sp),d0   ; p1 2
    move.l  .len(sp),d1   ; p1 3

    vperm   #$77777777,d0,e0,e0

    moveq   #64,d1
    sub.l   d1,d0
    bcs     .l32
.l64
    REPT    8
    store   e0,(a1)+
    ENDR
    sub.l   d1,d0
    bcc     .l64
.l32
    add.l   d1,d0
    beq     .exit2
    bclr    #5,d0
    beq.b   .l16
    REPT    4
    store   e0,(a1)+
    ENDR
.l16
    storec   e0,d0,(a1)+
    subq.l  #8,d0
    bcs.b   .exit2
    storec   e0,d0,(a1)+
.exit2
    move.l  .dst(sp),d0
.exit
    nop
* remove initial comparison so that it now only costs 1 cycle
    move.w  #$203c,.entry(pc)   ; move.l #nnnn,d0
    move.w  #$223c,.entry+6(pc) ; move.l #nnnn,d1
    move.w  #$4e75,.exit(pc)    ; #rts
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

* end of file