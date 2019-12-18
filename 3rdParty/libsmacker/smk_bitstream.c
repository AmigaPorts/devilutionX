/**
    libsmacker - A C library for decoding .smk Smacker Video files
    Copyright (C) 2012-2017 Greg Kennedy

    See smacker.h for more information.

    smk_bitstream.c
        Implements a bitstream structure, which can extract and
        return a bit at a time from a raw block of bytes.
*/

#include "smk_bitstream.h"

/* malloc and friends */
#include "smk_malloc.h"

#if 1 /* SAM's version */
// #undef __mc68000__ // to test C version

struct smk_bit_t
{
    unsigned short buf;
    unsigned char *ptr, *end_m1;
    unsigned long  siz;
};

struct smk_bit_t* smk_bs_init(const unsigned char* b, const unsigned long size)
{
    struct smk_bit_t* ret = NULL;

    /* sanity check */
    smk_assert(b);

    /* allocate a bitstream struct */
    smk_malloc(ret, sizeof(struct smk_bit_t));

    /* set up the pointer to bitstream, and the size counter */
    ret->buf    = 1;
    ret->ptr    = b;
    ret->end_m1 = b + size - 1;
    ret->siz    = size;

    /* point to initial byte: note, smk_malloc already sets these to 0 */
    /* ret->byte_num = 0;
    ret->bit_num = 0; */

    /* return ret or NULL if error : ) */
error:
    return ret;
}

REGPARM unsigned char _smk_error(struct smk_bit_t* bs)
{
    fprintf(stderr, "libsmacker::_smk_bs_read_?(bs=%p): ERROR: bitstream (length=%lu, ptr=%p, end=%p) exhausted.\n", bs, bs->siz, bs->ptr, bs->end_m1+1);
    bs->buf=1;
    return -1;
}

/* Reads a bit
    Returns -1 if error encountered */
REGPARM char _smk_bs_read_1(struct smk_bit_t* bs)
{
    /* sanity check */
    //  smk_assert(bs);
    {
#ifdef __mc68000__
    register unsigned char ret     asm("d0");
    register struct smk_bit_t* bs_ asm("a0") = bs;
    __asm__ __volatile__ (
    "   moveq   #0,d0       \n"
    "   lsr.w   (a0)        \n"
    "   bne.b   .result%=   \n"
    "   move.l  2(a0),a1    \n"
    "   cmp.l   6(a0),a1    \n"
#ifdef __PROFILE__
    "   bhi     __smk_error \n"
#else
    "   bhi.b   __smk_error \n"
#endif
    "   bne.b   .get_two%=  \n"
    ".only_one%=:           \n"
    "   move.w  #256,d1     \n"
    "   move.b  (a1)+,d1    \n"
    "   lsr.w   #1,d1       \n"
    "   bra.b   .set_buf%=  \n"
    ".get_two%=:            \n"
    "   moveq   #-1,d1      \n"
    "   move.w  (a1)+,d1    \n"
    "   ror.w   #8,d1       \n"
    "   lsr.l   #1,d1       \n"
    ".set_buf%=:            \n"
    "   move.w  d1,(a0)     \n"
    "   move.l  a1,2(a0)    \n"
    ".result%=:             \n"
    "   addx.l  d0,d0       \n"
    : "=d" (ret) : "a" (bs_)
    : "d1","a1","a0" );
    return ret;
#else
    unsigned short ret;

    ret = bs->buf; bs->buf >>= 1;
    if(!bs->buf) {
        if(bs->ptr >  bs->end_m1) return _smk_error(bs);
        if(bs->ptr == bs->end_m1) { // only 1 byte remaining in stream
            ret = 256; ret |= *bs->ptr++;
            bs->buf = ret>>1;
        } else {
            ret  =  *bs->ptr++;
            ret |= (*bs->ptr++)<<8;
            bs->buf = (ret>>1)|(unsigned short)32768;
        }
    } 
    return ret & 1;
#endif
    }
}

/* Reads a byte
    Returns -1 if error. */
REGPARM short _smk_bs_read_8(struct smk_bit_t* bs)
{
    /* sanity check */
    //  smk_assert(bs);
    {
#ifdef __mc68000__
    register unsigned char ret     asm("d0");
    register struct smk_bit_t* bs_ asm("a0") = bs;
    
    __asm__ __volatile__ (
    "   move.l  2(a0),a1    \n"
    "   cmp.l   6(a0),a1    \n"
#ifdef __PROFILE__
    "   bhi     __smk_error \n"
#else
    // "   bhi.b   __smk_error \n"
#endif
    // a = bs->buf
    "   moveq   #0,d0       \n"
    "   move.w  (a0),d0     \n"
    // a <= 1 ?
    "   moveq   #1,d1       \n"
    "   cmp.w   d1,d0       \n"
    "   bhi.b   .l1.%=      \n"
    // yes ==> return *bs-ptr++
    "   addq.l  #1,2(a0)    \n"
    "   move.b  (a1),d0     \n"
    "   bra.b   .xit%=      \n"
    ".l1.%=:                \n"
    // a < 256 ?
    "   cmp.w   #256,d0     \n"
    "   bcs.b   .l2.%=      \n"
    // no ==> more than 1 byte left, extract it
    "   exg     d0,d1       \n"
    "   bra.b   .l3.%=      \n"
    ".l2.%=:                \n"
    // yes ==> inject next byte
    "   addq.l  #1,2(a0)    \n"
    "   swap    d1          \n"
    "   move.w  (a1),d1     \n"
    "   move.b  d0,d1       \n"
    "   bfffo   d0{24:8},d0 \n"
    "   sub.w   #23,d0      \n"
    "   lsl.b   d0,d1       \n"
    "   lsr.l   d0,d1       \n"
    ".l3.%=:                \n"
    "   move.b  d1,d0       \n"
    "   lsr.l   #8,d1       \n"
    "   move.w  d1,(a0)     \n"
    ".xit%=:                \n"
    : "=d" (ret) : "a" (bs_)
    : "d1","a1","a0");
#else
    unsigned char ret; unsigned short a;

    if(bs->ptr > bs->end_m1) return _smk_error(bs);
    
    // aligned
    a = bs->buf;
    if(a <= 1) return *bs->ptr++;
    
    // more than 1 byte left
    a = bs->buf;
    if(a>=256) {
        ret = a;
        bs->buf = a>>8;
    } else {
        // find leftmost bit
        ret = a; a |= a>>1; a |= a>>2; a |= a>>4; a >>= 1; a += 1;

        // remove it from current buffer
        ret ^= a;
        
        // shift next byte + setup sentinel
        a *= *bs->ptr++ | (unsigned short)256;
        
        // inject current
        a |= ret;
        
        // setup result + shift buffer
        ret = a; bs->buf = a>>8;
    }
    
    return ret;
#endif
    }
}

#else

/*
    Bitstream structure
    Pointer to raw block of data and a size limit.
    Maintains internal pointers to byte_num and bit_number.
*/
struct smk_bit_t
{
    const unsigned char* buffer;
    unsigned long size;

    unsigned long byte_num;
    char bit_num;
};

/* BITSTREAM Functions */
struct smk_bit_t* smk_bs_init(const unsigned char* b, const unsigned long size)
{
    struct smk_bit_t* ret = NULL;

    /* sanity check */
    smk_assert(b);

    /* allocate a bitstream struct */
    smk_malloc(ret, sizeof(struct smk_bit_t));

    /* set up the pointer to bitstream, and the size counter */
    ret->buffer = b;
    ret->size = size;

    /* point to initial byte: note, smk_malloc already sets these to 0 */
    /* ret->byte_num = 0;
    ret->bit_num = 0; */

    /* return ret or NULL if error : ) */
error:
    return ret;
}

/* Reads a bit
    Returns -1 if error encountered */
char _smk_bs_read_1(struct smk_bit_t* bs)
{
    unsigned char ret = -1;

    /* sanity check */
    smk_assert(bs);

    /* don't die when running out of bits, but signal */
    if (bs->byte_num >= bs->size)
    {
        fprintf(stderr, "libsmacker::_smk_bs_read_1(bs): ERROR: bitstream (length=%lu) exhausted.\n", bs->size);
        goto error;
    }

    /* get next bit and return */
    ret = (((bs->buffer[bs->byte_num]) & (1 << bs->bit_num)) != 0);

    /* advance to next bit */
    bs->bit_num ++;

    /* Out of bits in this byte: next! */
    if (bs->bit_num > 7)
    {
        bs->byte_num ++;
        bs->bit_num = 0;
    }

    /* return ret, or (default) -1 if error */
error:
    return ret;
}

/* Reads a byte
    Returns -1 if error. */
short _smk_bs_read_8(struct smk_bit_t* bs)
{
    unsigned char ret = -1;

    /* sanity check */
    smk_assert(bs);

    /* don't die when running out of bits, but signal */
    if (bs->byte_num + (bs->bit_num > 0) >= bs->size)
    {
        fprintf(stderr, "libsmacker::_smk_bs_read_8(bs): ERROR: bitstream (length=%lu) exhausted.\n", bs->size);
        goto error;
    }

    if (bs->bit_num)
    {
        /* unaligned read */
        ret = bs->buffer[bs->byte_num] >> bs->bit_num;
        bs->byte_num ++;
        ret |= (bs->buffer[bs->byte_num] << (8 - bs->bit_num));
    } else {
        /* aligned read */
        ret = bs->buffer[bs->byte_num ++];
    }

    /* return ret, or (default) -1 if error */
error:
    return ret;
}

#endif