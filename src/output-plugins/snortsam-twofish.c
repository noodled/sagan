/* $Id: twofish.c,v 2.1 2008/12/15 20:36:05 fknobbe Exp $
 *
 *
 * Copyright (C) 1997-2000 The Cryptix Foundation Limited.
 * Copyright (C) 2000 Farm9.
 * Copyright (C) 2001 Frank Knobbe.
 * All rights reserved.
 *
 * For Cryptix code:
 * Use, modification, copying and distribution of this software is subject
 * the terms and conditions of the Cryptix General Licence. You should have
 * received a copy of the Cryptix General Licence along with this library;
 * if not, you can download a copy from http://www.cryptix.org/ .
 *
 * For Farm9:
 * ---  jojo@farm9.com, August 2000, converted from Java to C++, added CBC mode and
 *      ciphertext stealing technique, added AsciiTwofish class for easy encryption
 *      decryption of text strings
 *
 * Frank Knobbe <frank@knobbe.us>:
 * ---  April 2001, converted from C++ to C, prefixed global variables
 *      with TwoFish, substituted some defines, changed functions to make use of
 *      variables supplied in a struct, modified and added routines for modular calls.
 *      Cleaned up the code so that defines are used instead of fixed 16's and 32's.
 *      Created two general purpose crypt routines for one block and multiple block
 *      encryption using Joh's CBC code.
 *		Added crypt routines that use a header (with a magic and data length).
 *		(Basically a major rewrite).
 *
 *      Note: Routines labeled _TwoFish are private and should not be used
 *      (or with extreme caution).
 *
 */

#ifndef __TWOFISH_LIBRARY_SOURCE__
#define __TWOFISH_LIBRARY_SOURCE__

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/types.h>

#ifdef WIN32

#ifndef u_long
typedef unsigned long u_long;
#endif
#ifndef u_int32_t
typedef unsigned long u_int32_t;
#endif
#ifndef u_word
typedef unsigned short u_word;
#endif
#ifndef u_int16_t
typedef unsigned short u_int16_t;
#endif
#ifndef u_char
typedef unsigned char u_char;
#endif
#ifndef u_int8_t
typedef unsigned char u_int8_t;
#endif

#endif /* WIN32 */

#include "output-plugins/snortsam-twofish.h"


bool TwoFish_srand=true;				/* if TRUE, first call of TwoFishInit will seed rand(); */
/* of TwoFishInit */

/* Fixed 8x8 permutation S-boxes */
static const u_int8_t TwoFish_P[2][256] =
{
    {  /* p0 */
        0xA9, 0x67, 0xB3, 0xE8,   0x04, 0xFD, 0xA3, 0x76,   0x9A, 0x92, 0x80, 0x78,
        0xE4, 0xDD, 0xD1, 0x38,   0x0D, 0xC6, 0x35, 0x98,   0x18, 0xF7, 0xEC, 0x6C,
        0x43, 0x75, 0x37, 0x26,   0xFA, 0x13, 0x94, 0x48,   0xF2, 0xD0, 0x8B, 0x30,
        0x84, 0x54, 0xDF, 0x23,   0x19, 0x5B, 0x3D, 0x59,   0xF3, 0xAE, 0xA2, 0x82,
        0x63, 0x01, 0x83, 0x2E,   0xD9, 0x51, 0x9B, 0x7C,   0xA6, 0xEB, 0xA5, 0xBE,
        0x16, 0x0C, 0xE3, 0x61,   0xC0, 0x8C, 0x3A, 0xF5,   0x73, 0x2C, 0x25, 0x0B,
        0xBB, 0x4E, 0x89, 0x6B,   0x53, 0x6A, 0xB4, 0xF1,   0xE1, 0xE6, 0xBD, 0x45,
        0xE2, 0xF4, 0xB6, 0x66,   0xCC, 0x95, 0x03, 0x56,   0xD4, 0x1C, 0x1E, 0xD7,
        0xFB, 0xC3, 0x8E, 0xB5,   0xE9, 0xCF, 0xBF, 0xBA,   0xEA, 0x77, 0x39, 0xAF,
        0x33, 0xC9, 0x62, 0x71,   0x81, 0x79, 0x09, 0xAD,   0x24, 0xCD, 0xF9, 0xD8,
        0xE5, 0xC5, 0xB9, 0x4D,   0x44, 0x08, 0x86, 0xE7,   0xA1, 0x1D, 0xAA, 0xED,
        0x06, 0x70, 0xB2, 0xD2,   0x41, 0x7B, 0xA0, 0x11,   0x31, 0xC2, 0x27, 0x90,
        0x20, 0xF6, 0x60, 0xFF,   0x96, 0x5C, 0xB1, 0xAB,   0x9E, 0x9C, 0x52, 0x1B,
        0x5F, 0x93, 0x0A, 0xEF,   0x91, 0x85, 0x49, 0xEE,   0x2D, 0x4F, 0x8F, 0x3B,
        0x47, 0x87, 0x6D, 0x46,   0xD6, 0x3E, 0x69, 0x64,   0x2A, 0xCE, 0xCB, 0x2F,
        0xFC, 0x97, 0x05, 0x7A,   0xAC, 0x7F, 0xD5, 0x1A,   0x4B, 0x0E, 0xA7, 0x5A,
        0x28, 0x14, 0x3F, 0x29,   0x88, 0x3C, 0x4C, 0x02,   0xB8, 0xDA, 0xB0, 0x17,
        0x55, 0x1F, 0x8A, 0x7D,   0x57, 0xC7, 0x8D, 0x74,   0xB7, 0xC4, 0x9F, 0x72,
        0x7E, 0x15, 0x22, 0x12,   0x58, 0x07, 0x99, 0x34,   0x6E, 0x50, 0xDE, 0x68,
        0x65, 0xBC, 0xDB, 0xF8,   0xC8, 0xA8, 0x2B, 0x40,   0xDC, 0xFE, 0x32, 0xA4,
        0xCA, 0x10, 0x21, 0xF0,   0xD3, 0x5D, 0x0F, 0x00,   0x6F, 0x9D, 0x36, 0x42,
        0x4A, 0x5E, 0xC1, 0xE0
    },
    {  /* p1 */
        0x75, 0xF3, 0xC6, 0xF4,   0xDB, 0x7B, 0xFB, 0xC8,   0x4A, 0xD3, 0xE6, 0x6B,
        0x45, 0x7D, 0xE8, 0x4B,   0xD6, 0x32, 0xD8, 0xFD,   0x37, 0x71, 0xF1, 0xE1,
        0x30, 0x0F, 0xF8, 0x1B,   0x87, 0xFA, 0x06, 0x3F,   0x5E, 0xBA, 0xAE, 0x5B,
        0x8A, 0x00, 0xBC, 0x9D,   0x6D, 0xC1, 0xB1, 0x0E,   0x80, 0x5D, 0xD2, 0xD5,
        0xA0, 0x84, 0x07, 0x14,   0xB5, 0x90, 0x2C, 0xA3,   0xB2, 0x73, 0x4C, 0x54,
        0x92, 0x74, 0x36, 0x51,   0x38, 0xB0, 0xBD, 0x5A,   0xFC, 0x60, 0x62, 0x96,
        0x6C, 0x42, 0xF7, 0x10,   0x7C, 0x28, 0x27, 0x8C,   0x13, 0x95, 0x9C, 0xC7,
        0x24, 0x46, 0x3B, 0x70,   0xCA, 0xE3, 0x85, 0xCB,   0x11, 0xD0, 0x93, 0xB8,
        0xA6, 0x83, 0x20, 0xFF,   0x9F, 0x77, 0xC3, 0xCC,   0x03, 0x6F, 0x08, 0xBF,
        0x40, 0xE7, 0x2B, 0xE2,   0x79, 0x0C, 0xAA, 0x82,   0x41, 0x3A, 0xEA, 0xB9,
        0xE4, 0x9A, 0xA4, 0x97,   0x7E, 0xDA, 0x7A, 0x17,   0x66, 0x94, 0xA1, 0x1D,
        0x3D, 0xF0, 0xDE, 0xB3,   0x0B, 0x72, 0xA7, 0x1C,   0xEF, 0xD1, 0x53, 0x3E,
        0x8F, 0x33, 0x26, 0x5F,   0xEC, 0x76, 0x2A, 0x49,   0x81, 0x88, 0xEE, 0x21,
        0xC4, 0x1A, 0xEB, 0xD9,   0xC5, 0x39, 0x99, 0xCD,   0xAD, 0x31, 0x8B, 0x01,
        0x18, 0x23, 0xDD, 0x1F,   0x4E, 0x2D, 0xF9, 0x48,   0x4F, 0xF2, 0x65, 0x8E,
        0x78, 0x5C, 0x58, 0x19,   0x8D, 0xE5, 0x98, 0x57,   0x67, 0x7F, 0x05, 0x64,
        0xAF, 0x63, 0xB6, 0xFE,   0xF5, 0xB7, 0x3C, 0xA5,   0xCE, 0xE9, 0x68, 0x44,
        0xE0, 0x4D, 0x43, 0x69,   0x29, 0x2E, 0xAC, 0x15,   0x59, 0xA8, 0x0A, 0x9E,
        0x6E, 0x47, 0xDF, 0x34,   0x35, 0x6A, 0xCF, 0xDC,   0x22, 0xC9, 0xC0, 0x9B,
        0x89, 0xD4, 0xED, 0xAB,   0x12, 0xA2, 0x0D, 0x52,   0xBB, 0x02, 0x2F, 0xA9,
        0xD7, 0x61, 0x1E, 0xB4,   0x50, 0x04, 0xF6, 0xC2,   0x16, 0x25, 0x86, 0x56,
        0x55, 0x09, 0xBE, 0x91
    }
};

static bool TwoFish_MDSready=false;
static u_int32_t TwoFish_MDS[4][256]; /* TwoFish_MDS matrix */


#define	TwoFish_LFSR1(x) (((x)>>1)^(((x)&0x01)?TwoFish_MDS_GF_FDBK/2:0))
#define	TwoFish_LFSR2(x) (((x)>>2)^(((x)&0x02)?TwoFish_MDS_GF_FDBK/2:0)^(((x)&0x01)?TwoFish_MDS_GF_FDBK/4:0))

#define	TwoFish_Mx_1(x) ((u_int32_t)(x))		/* force result to dword so << will work  */
#define	TwoFish_Mx_X(x) ((u_int32_t)((x)^TwoFish_LFSR2(x)))	/* 5B */
#define	TwoFish_Mx_Y(x) ((u_int32_t)((x)^TwoFish_LFSR1(x)^TwoFish_LFSR2(x)))	/* EF  */
#define	TwoFish_RS_rem(x) { u_int8_t b=(u_int8_t)(x>>24); u_int32_t g2=((b<<1)^((b&0x80)?TwoFish_RS_GF_FDBK:0))&0xFF; u_int32_t g3=((b>>1)&0x7F)^((b&1)?TwoFish_RS_GF_FDBK>>1:0)^g2; x=(x<<8)^(g3<<24)^(g2<<16)^(g3<<8)^b; }

/*#define	TwoFish__b(x,N)	(((u_int8_t *)&x)[((N)&3)^TwoFish_ADDR_XOR])*/ /* pick bytes out of a dword */

#define	TwoFish_b0(x)			TwoFish__b(x,0)		/* extract LSB of u_int32_t  */
#define	TwoFish_b1(x)			TwoFish__b(x,1)
#define	TwoFish_b2(x)			TwoFish__b(x,2)
#define	TwoFish_b3(x)			TwoFish__b(x,3)		/* extract MSB of u_int32_t  */

u_int8_t TwoFish__b(u_int32_t x,int n)
{
    n&=3;
    while(n-->0)
        x>>=8;
    return (u_int8_t)x;
}


/*	TwoFish Initialization
 *
 *	This routine generates a global data structure for use with TwoFish,
 *	initializes important values (such as subkeys, sBoxes), generates subkeys
 *	and precomputes the MDS matrix if not already done.
 *
 *	Input:	User supplied password (will be appended by default password of 'SnortHas2FishEncryptionRoutines!')
 *
 *  Output:	Pointer to TWOFISH structure. This data structure contains key dependent data.
 *			This pointer is used with all other crypt functions.
 */

TWOFISH *TwoFishInit(char *userkey)
{
    TWOFISH *tfdata;
    int i,x,m;
    char tkey[TwoFish_KEY_LENGTH+40];

    tfdata=malloc(sizeof(TWOFISH));			/* allocate the TwoFish structure */
    if(tfdata!=NULL)
        {
            if(*userkey)
                {
                    strncpy(tkey,userkey,TwoFish_KEY_LENGTH);			/* use first 32 chars of user supplied password */
                    tkey[TwoFish_KEY_LENGTH]=0;							/* make sure it wasn't more */
                }
            else
                strcpy(tkey,TwoFish_DEFAULT_PW);	/* if no key defined, use default password */
            for(i=0,x=0,m=strlen(tkey); i<TwoFish_KEY_LENGTH; i++)  	/* copy into data structure */
                {
                    tfdata->key[i]=tkey[x++];							/* fill the whole keyspace with repeating key. */
                    if(x==m)
                        x=0;
                }

            if(!TwoFish_MDSready)
                _TwoFish_PrecomputeMDSmatrix();		/* "Wake Up, Neo" */
            _TwoFish_MakeSubKeys(tfdata);			/* generate subkeys */
            _TwoFish_ResetCBC(tfdata);				/* reset the CBC */
            tfdata->output=NULL;					/* nothing to output yet */
            tfdata->dontflush=false;				/* reset decrypt skip block flag */
            if(TwoFish_srand)
                {
                    TwoFish_srand=false;
                    srand(time(NULL));
                }
        }
    return tfdata;							/* return the data pointer */
}


void TwoFishDestroy(TWOFISH *tfdata)
{
    if(tfdata!=NULL)
        free(tfdata);
}


/* en/decryption with CBC mode */
unsigned long _TwoFish_CryptRawCBC(char *in,char *out,unsigned long len,bool decrypt,TWOFISH *tfdata)
{
    unsigned long rl;

    rl=len;											/* remember how much data to crypt. */
    while(len>TwoFish_BLOCK_SIZE)  				/* and now we process block by block. */
        {
            _TwoFish_BlockCrypt((unsigned char*)in,(unsigned char*)out,TwoFish_BLOCK_SIZE,decrypt,tfdata); /* de/encrypt it. */
            in+=TwoFish_BLOCK_SIZE;						/* adjust pointers. */
            out+=TwoFish_BLOCK_SIZE;
            len-=TwoFish_BLOCK_SIZE;
        }
    if(len>0)										/* if we have less than a block left... */
        _TwoFish_BlockCrypt( (unsigned char*)in, (unsigned char*)out,len,decrypt,tfdata);	/* ...then we de/encrypt that too. */
    if(tfdata->qBlockDefined && !tfdata->dontflush)						/* in case len was exactly one block... */
        _TwoFish_FlushOutput(tfdata->qBlockCrypt,TwoFish_BLOCK_SIZE,tfdata); /* ...we need to write the...  */
    /* ...remaining bytes of the buffer */
    return rl;
}

/* en/decryption on one block only */
unsigned long _TwoFish_CryptRaw16(char *in,char *out,unsigned long len,bool decrypt,TWOFISH *tfdata)
{
    /* qBlockPlain already zero'ed through ResetCBC  */
    memcpy(tfdata->qBlockPlain,in,len);					/* toss the data into it. */
    _TwoFish_BlockCrypt16(tfdata->qBlockPlain,tfdata->qBlockCrypt,decrypt,tfdata); /* encrypt just that block without CBC. */
    memcpy(out,tfdata->qBlockCrypt,TwoFish_BLOCK_SIZE);				/* and return what we got */
    return TwoFish_BLOCK_SIZE;
}

/* en/decryption without reset of CBC and output assignment */
unsigned long _TwoFish_CryptRaw(char *in,char *out,unsigned long len,bool decrypt,TWOFISH *tfdata)
{
    if(in!=NULL && out!=NULL && len>0 && tfdata!=NULL)  	/* if we have valid data, then... */
        {
            if(len>TwoFish_BLOCK_SIZE)							/* ...check if we have more than one block. */
                return _TwoFish_CryptRawCBC(in,out,len,decrypt,tfdata); /* if so, use the CBC routines... */
            else
                return _TwoFish_CryptRaw16(in,out,len,decrypt,tfdata); /* ...otherwise just do one block. */
        }
    return 0;
}


/*	TwoFish Raw Encryption
 *
 *	Does not use header, but does use CBC (if more than one block has to be encrypted).
 *
 *	Input:	Pointer to the buffer of the plaintext to be encrypted.
 *			Pointer to the buffer receiving the ciphertext.
 *			The length of the plaintext buffer.
 *			The TwoFish structure.
 *
 *	Output:	The amount of bytes encrypted if successful, otherwise 0.
 */

unsigned long TwoFishEncryptRaw(char *in,
                                char *out,
                                unsigned long len,
                                TWOFISH *tfdata)
{
    _TwoFish_ResetCBC(tfdata);							/* reset CBC flag. */
    tfdata->output=(unsigned char*)out;							/* output straight into output buffer. */
    return _TwoFish_CryptRaw(in,out,len,false,tfdata);	/* and go for it. */
}

/*	TwoFish Raw Decryption
 *
 *	Does not use header, but does use CBC (if more than one block has to be decrypted).
 *
 *	Input:	Pointer to the buffer of the ciphertext to be decrypted.
 *			Pointer to the buffer receiving the plaintext.
 *			The length of the ciphertext buffer (at least one cipher block).
 *			The TwoFish structure.
 *
 *	Output:	The amount of bytes decrypted if successful, otherwise 0.
 */

unsigned long TwoFishDecryptRaw(char *in,
                                char *out,
                                unsigned long len,
                                TWOFISH *tfdata)
{
    _TwoFish_ResetCBC(tfdata);							/* reset CBC flag. */
    tfdata->output=(unsigned char*)out;							/* output straight into output buffer. */
    return _TwoFish_CryptRaw(in,out,len,true,tfdata);	/* and go for it. */
}

/*	TwoFish Free
 *
 *	Free's the allocated buffer.
 *
 *	Input:	Pointer to the TwoFish structure
 *
 *	Output:	(none)
 */

void TwoFishFree(TWOFISH *tfdata)
{
    if(tfdata->output!=NULL)  	/* if a valid buffer is present... */
        {
            free(tfdata->output);	/* ...then we free it for you... */
            tfdata->output=NULL;	/* ...and mark as such. */
        }
}

/*	TwoFish Set Output
 *
 *	If you want to allocate the output buffer yourself,
 *	then you can set it with this function.
 *
 *	Input:	Pointer to your output buffer
 *			Pointer to the TwoFish structure
 *
 *	Output:	(none)
 */

void TwoFishSetOutput(char *outp,TWOFISH *tfdata)
{
    tfdata->output=(unsigned char*)outp;				/* (do we really need a function for this?) */
}

/*	TwoFish Alloc
 *
 *	Allocates enough memory for the output buffer that would be required
 *
 *	Input:	Length of the plaintext.
 *			Boolean flag for BinHex Output.
 *			Pointer to the TwoFish structure.
 *
 *	Output:	Returns a pointer to the memory allocated.
 */

void *TwoFishAlloc(unsigned long len,bool binhex,bool decrypt,TWOFISH *tfdata)
{
    /*	TwoFishFree(tfdata);	*/			/* (don't for now) discard whatever was allocated earlier. */
    if(decrypt)  						/* if decrypting... */
        {
            if(binhex)						/* ...and input is binhex encoded... */
                len/=2;						/* ...use half as much for output. */
            len-=TwoFish_BLOCK_SIZE;		/* Also, subtract the size of the header. */
        }
    else
        {
            len+=TwoFish_BLOCK_SIZE;		/* the size is just increased by the header... */
            if(binhex)
                len*=2;						/* ...and doubled if output is to be binhexed. */
        }
    tfdata->output=malloc(len+TwoFish_BLOCK_SIZE);/* grab some memory...plus some extra (it's running over somewhere, crashes without extra padding) */

    return tfdata->output;				/* ...and return to caller. */
}

/* bin2hex and hex2bin conversion */
void _TwoFish_BinHex(u_int8_t *buf,unsigned long len,bool bintohex)
{
    u_int8_t *pi,*po,c;

    if(bintohex)
        {
            for(pi=buf+len-1,po=buf+(2*len)-1; len>0; pi--,po--,len--)   /* let's start from the end of the bin block. */
                {
                    c=*pi;												 /* grab value. */
                    c&=15;												 /* use lower 4 bits. */
                    if(c>9)												 /* convert to ascii. */
                        c+=('a'-10);
                    else
                        c+='0';
                    *po--=c;											 /* set the lower nibble. */
                    c=*pi;												 /* grab value again. */
                    c>>=4;												 /* right shift 4 bits. */
                    c&=15;												 /* make sure we only have 4 bits. */
                    if(c>9)												 /* convert to ascii. */
                        c+=('a'-10);
                    else
                        c+='0';
                    *po=c;												 /* set the higher nibble. */
                }														 /* and keep going. */
        }
    else
        {
            for(pi=buf,po=buf; len>0; pi++,po++,len-=2)  			 /* let's start from the beginning of the hex block. */
                {
                    c=tolower(*pi++)-'0';								 /* grab higher nibble. */
                    if(c>9)												 /* convert to value. */
                        c-=('0'-9);
                    *po=c<<4;											 /* left shit 4 bits. */
                    c=tolower(*pi)-'0';									 /* grab lower nibble. */
                    if(c>9)												 /* convert to value. */
                        c-=('0'-9);
                    *po|=c;												 /* and add to value. */
                }
        }
}


/*	TwoFish Encryption
 *
 *	Uses header and CBC. If the output area has not been intialized with TwoFishAlloc,
 *  this routine will alloc the memory. In addition, it will include a small 'header'
 *  containing the magic and some salt. That way the decrypt routine can check if the
 *  packet got decrypted successfully, and return 0 instead of garbage.
 *
 *	Input:	Pointer to the buffer of the plaintext to be encrypted.
 *			Pointer to the pointer to the buffer receiving the ciphertext.
 *				The pointer either points to user allocated output buffer space, or to NULL, in which case
 *				this routine will set the pointer to the buffer allocated through the struct.
 *			The length of the plaintext buffer.
 *				Can be -1 if the input is a null terminated string, in which case we'll count for you.
 *			Boolean flag for BinHex Output (if used, output will be twice as large as input).
 *				Note: BinHex conversion overwrites (converts) input buffer!
 *			The TwoFish structure.
 *
 *	Output:	The amount of bytes encrypted if successful, otherwise 0.
 */

unsigned long TwoFishEncrypt(char *in,
                             char **out,
                             signed long len,
                             bool binhex,
                             TWOFISH *tfdata)
{
    unsigned long ilen,olen;


    if(len== -1)			/* if we got -1 for len, we'll assume IN is a...  */
        ilen=strlen(in);	/* ...\0 terminated string and figure len out ourselves... */
    else
        ilen=len;			/* ...otherwise we trust you supply a correct length. */

    if(in!=NULL && out!=NULL && ilen>0 && tfdata!=NULL)   /* if we got usable stuff, we'll do it. */
        {
            if(*out==NULL)									/* if OUT points to a NULL pointer... */
                *out=TwoFishAlloc(ilen,binhex,false,tfdata);  /* ...we'll (re-)allocate buffer space. */
            if(*out!=NULL)
                {
                    tfdata->output=(unsigned char*)*out;							/* set output buffer. */
                    tfdata->header.salt=rand()*65536+rand();		/* toss in some salt. */
                    tfdata->header.length[0]= (u_int8_t)(ilen);
                    tfdata->header.length[1]= (u_int8_t)(ilen>>8);
                    tfdata->header.length[2]= (u_int8_t)(ilen>>16);
                    tfdata->header.length[3]= (u_int8_t)(ilen>>24);
                    memcpy(tfdata->header.magic,TwoFish_MAGIC,TwoFish_MAGIC_LEN); /* set the magic. */
                    olen=TwoFish_BLOCK_SIZE;						/* set output counter. */
                    _TwoFish_ResetCBC(tfdata);						/* reset the CBC flag */
                    _TwoFish_BlockCrypt((u_int8_t *)&(tfdata->header),(unsigned char*)*out,olen,false,tfdata); /* encrypt first block (without flush on 16 byte boundary). */
                    olen+=_TwoFish_CryptRawCBC(in,*out+TwoFish_BLOCK_SIZE,ilen,false,tfdata);	/* and encrypt the rest (we do not reset the CBC flag). */
                    if(binhex)  								/* if binhex... */
                        {
                            _TwoFish_BinHex( (unsigned char*)*out,olen,true);		/* ...convert output to binhex... */
                            olen*=2;								/* ...and size twice as large. */
                        }
                    tfdata->output=(unsigned char*)*out;
                    return olen;
                }
        }
    return 0;
}

/*	TwoFish Decryption
 *
 *	Uses header and CBC. If the output area has not been intialized with TwoFishAlloc,
 *  this routine will alloc the memory. In addition, it will check the small 'header'
 *  containing the magic. If magic does not match we return 0. Otherwise we return the
 *  amount of bytes decrypted (should be the same as the length in the header).
 *
 *	Input:	Pointer to the buffer of the ciphertext to be decrypted.
 *			Pointer to the pointer to the buffer receiving the plaintext.
 *				The pointer either points to user allocated output buffer space, or to NULL, in which case
 *				this routine will set the pointer to the buffer allocated through the struct.
 *			The length of the ciphertext buffer.
 *				Can be -1 if the input is a null terminated binhex string, in which case we'll count for you.
 *			Boolean flag for BinHex Input (if used, plaintext will be half as large as input).
 *				Note: BinHex conversion overwrites (converts) input buffer!
 *			The TwoFish structure.
 *
 *	Output:	The amount of bytes decrypted if successful, otherwise 0.
 */

unsigned long TwoFishDecrypt(char *in,
                             char **out,
                             signed long len,
                             bool binhex,
                             TWOFISH *tfdata)
{
    unsigned long ilen,elen,olen;
    const u_int8_t cmagic[TwoFish_MAGIC_LEN]=TwoFish_MAGIC;
    u_int8_t *tbuf;



    if(len== -1)			/* if we got -1 for len, we'll assume IN is...  */
        ilen=strlen(in);	/* ...\0 terminated binhex and figure len out ourselves... */
    else
        ilen=len;			/* ...otherwise we trust you supply a correct length. */

    if(in!=NULL && out!=NULL && ilen>0 && tfdata!=NULL)   /* if we got usable stuff, we'll do it. */
        {
            if(*out==NULL)									/* if OUT points to a NULL pointer... */
                *out=TwoFishAlloc(ilen,binhex,true,tfdata); /* ...we'll (re-)allocate buffer space. */
            if(*out!=NULL)
                {
                    if(binhex)  								/* if binhex... */
                        {
                            _TwoFish_BinHex( (unsigned char*)in,ilen,false);		/* ...convert input to values... */
                            ilen/=2;								/* ...and size half as much. */
                        }
                    _TwoFish_ResetCBC(tfdata);						/* reset the CBC flag. */

                    tbuf=(u_int8_t *)malloc(ilen+TwoFish_BLOCK_SIZE); /* get memory for data and header. */
                    if(tbuf==NULL)
                        return 0;
                    tfdata->output=tbuf;					/* set output to temp buffer. */

                    olen=_TwoFish_CryptRawCBC(in,(char *)tbuf,ilen,true,tfdata)-TwoFish_BLOCK_SIZE; /* decrypt the whole thing. */
                    memcpy(&(tfdata->header),tbuf,TwoFish_BLOCK_SIZE); /* copy first block into header. */
                    //tfdata->output=*out;
                    tfdata->output=(unsigned char*)*out;
                    for(elen=0; elen<TwoFish_MAGIC_LEN; elen++)	/* compare magic. */
                        if(tfdata->header.magic[elen]!=cmagic[elen])
                            break;
                    if(elen==TwoFish_MAGIC_LEN)  				/* if magic matches then... */
                        {
                            elen=(tfdata->header.length[0]) |
                                 (tfdata->header.length[1])<<8 |
                                 (tfdata->header.length[2])<<16 |
                                 (tfdata->header.length[3])<<24;	/* .. we know how much to expect. */
                            if(elen>olen)							/* adjust if necessary. */
                                elen=olen;
                            memcpy(*out,tbuf+TwoFish_BLOCK_SIZE,elen);	/* copy data into intended output. */
                            free(tbuf);
                            return elen;
                        }
                    free(tbuf);
                }
        }
    return 0;
}

void _TwoFish_PrecomputeMDSmatrix(void)	/* precompute the TwoFish_MDS matrix */
{
    u_int32_t m1[2];
    u_int32_t mX[2];
    u_int32_t mY[2];
    u_int32_t i, j;

    for (i = 0; i < 256; i++)
        {
            j = TwoFish_P[0][i]       & 0xFF; /* compute all the matrix elements */
            m1[0] = j;
            mX[0] = TwoFish_Mx_X( j ) & 0xFF;
            mY[0] = TwoFish_Mx_Y( j ) & 0xFF;

            j = TwoFish_P[1][i]       & 0xFF;
            m1[1] = j;
            mX[1] = TwoFish_Mx_X( j ) & 0xFF;
            mY[1] = TwoFish_Mx_Y( j ) & 0xFF;

            TwoFish_MDS[0][i] = m1[TwoFish_P_00] | /* fill matrix w/ above elements */
                                mX[TwoFish_P_00] <<  8 |
                                mY[TwoFish_P_00] << 16 |
                                mY[TwoFish_P_00] << 24;
            TwoFish_MDS[1][i] = mY[TwoFish_P_10] |
                                mY[TwoFish_P_10] <<  8 |
                                mX[TwoFish_P_10] << 16 |
                                m1[TwoFish_P_10] << 24;
            TwoFish_MDS[2][i] = mX[TwoFish_P_20] |
                                mY[TwoFish_P_20] <<  8 |
                                m1[TwoFish_P_20] << 16 |
                                mY[TwoFish_P_20] << 24;
            TwoFish_MDS[3][i] = mX[TwoFish_P_30] |
                                m1[TwoFish_P_30] <<  8 |
                                mY[TwoFish_P_30] << 16 |
                                mX[TwoFish_P_30] << 24;
        }
    TwoFish_MDSready=true;
}


void _TwoFish_MakeSubKeys(TWOFISH *tfdata)	/* Expand a user-supplied key material into a session key. */
{
    u_int32_t k64Cnt    = TwoFish_KEY_LENGTH / 8;
    u_int32_t k32e[4]; /* even 32-bit entities */
    u_int32_t k32o[4]; /* odd 32-bit entities */
    u_int32_t sBoxKey[4];
    u_int32_t offset,i,j;
    u_int32_t A, B, q=0;
    u_int32_t k0,k1,k2,k3;
    u_int32_t b0,b1,b2,b3;

    /* split user key material into even and odd 32-bit entities and */
    /* compute S-box keys using (12, 8) Reed-Solomon code over GF(256) */


    for (offset=0,i=0,j=k64Cnt-1; i<4 && offset<TwoFish_KEY_LENGTH; i++,j--)
        {
            k32e[i] = tfdata->key[offset++];
            k32e[i]|= tfdata->key[offset++]<<8;
            k32e[i]|= tfdata->key[offset++]<<16;
            k32e[i]|= tfdata->key[offset++]<<24;
            k32o[i] = tfdata->key[offset++];
            k32o[i]|= tfdata->key[offset++]<<8;
            k32o[i]|= tfdata->key[offset++]<<16;
            k32o[i]|= tfdata->key[offset++]<<24;
            sBoxKey[j] = _TwoFish_RS_MDS_Encode( k32e[i], k32o[i] ); /* reverse order */
        }

    /* compute the round decryption subkeys for PHT. these same subkeys */
    /* will be used in encryption but will be applied in reverse order. */
    i=0;
    while(i < TwoFish_TOTAL_SUBKEYS)
        {
            A = _TwoFish_F32( k64Cnt, q, k32e ); /* A uses even key entities */
            q += TwoFish_SK_BUMP;

            B = _TwoFish_F32( k64Cnt, q, k32o ); /* B uses odd  key entities */
            q += TwoFish_SK_BUMP;

            B = B << 8 | B >> 24;

            A += B;
            tfdata->subKeys[i++] = A;           /* combine with a PHT */

            A += B;
            tfdata->subKeys[i++] = A << TwoFish_SK_ROTL | A >> (32-TwoFish_SK_ROTL);
        }

    /* fully expand the table for speed */
    k0 = sBoxKey[0];
    k1 = sBoxKey[1];
    k2 = sBoxKey[2];
    k3 = sBoxKey[3];

    for (i = 0; i < 256; i++)
        {
            b0 = b1 = b2 = b3 = i;
            switch (k64Cnt & 3)
                {
                case 1: /* 64-bit keys */
                    tfdata->sBox[      2*i  ] = TwoFish_MDS[0][(TwoFish_P[TwoFish_P_01][b0]) ^ TwoFish_b0(k0)];
                    tfdata->sBox[      2*i+1] = TwoFish_MDS[1][(TwoFish_P[TwoFish_P_11][b1]) ^ TwoFish_b1(k0)];
                    tfdata->sBox[0x200+2*i  ] = TwoFish_MDS[2][(TwoFish_P[TwoFish_P_21][b2]) ^ TwoFish_b2(k0)];
                    tfdata->sBox[0x200+2*i+1] = TwoFish_MDS[3][(TwoFish_P[TwoFish_P_31][b3]) ^ TwoFish_b3(k0)];
                    break;
                case 0: /* 256-bit keys (same as 4) */
                    b0 = (TwoFish_P[TwoFish_P_04][b0]) ^ TwoFish_b0(k3);
                    b1 = (TwoFish_P[TwoFish_P_14][b1]) ^ TwoFish_b1(k3);
                    b2 = (TwoFish_P[TwoFish_P_24][b2]) ^ TwoFish_b2(k3);
                    b3 = (TwoFish_P[TwoFish_P_34][b3]) ^ TwoFish_b3(k3);
                case 3:  /* 192-bit keys */
                    b0 = (TwoFish_P[TwoFish_P_03][b0]) ^ TwoFish_b0(k2);
                    b1 = (TwoFish_P[TwoFish_P_13][b1]) ^ TwoFish_b1(k2);
                    b2 = (TwoFish_P[TwoFish_P_23][b2]) ^ TwoFish_b2(k2);
                    b3 = (TwoFish_P[TwoFish_P_33][b3]) ^ TwoFish_b3(k2);
                case 2: /* 128-bit keys */
                    tfdata->sBox[      2*i  ]=
                        TwoFish_MDS[0][(TwoFish_P[TwoFish_P_01][(TwoFish_P[TwoFish_P_02][b0]) ^
                                        TwoFish_b0(k1)]) ^ TwoFish_b0(k0)];

                    tfdata->sBox[      2*i+1]=
                        TwoFish_MDS[1][(TwoFish_P[TwoFish_P_11][(TwoFish_P[TwoFish_P_12][b1]) ^
                                        TwoFish_b1(k1)]) ^ TwoFish_b1(k0)];

                    tfdata->sBox[0x200+2*i  ]=
                        TwoFish_MDS[2][(TwoFish_P[TwoFish_P_21][(TwoFish_P[TwoFish_P_22][b2]) ^
                                        TwoFish_b2(k1)]) ^ TwoFish_b2(k0)];

                    tfdata->sBox[0x200+2*i+1]=
                        TwoFish_MDS[3][(TwoFish_P[TwoFish_P_31][(TwoFish_P[TwoFish_P_32][b3]) ^
                                        TwoFish_b3(k1)]) ^ TwoFish_b3(k0)];
                }
        }
}


/**
 * Encrypt or decrypt exactly one block of plaintext in CBC mode.
 * Use "ciphertext stealing" technique described on pg. 196
 * of "Applied Cryptography" to encrypt the final partial
 * (i.e. <16 byte) block if necessary.
 *
 * jojo: the "ciphertext stealing" requires we read ahead and have
 * special handling for the last two blocks.  Because of this, the
 * output from the TwoFish algorithm is handled internally here.
 * It would be better to have a higher level handle this as well as
 * CBC mode.  Unfortunately, I've mixed the two together, which is
 * pretty crappy... The Java version separates these out correctly.
 *
 * fknobbe:	I have reduced the CBC mode to work on memory buffer only.
 *			Higher routines should use an intermediate buffer and handle
 *			their output seperately (mainly so the data can be flushed
 *			in one chunk, not seperate 16 byte blocks...)
 *
 * @param in   The plaintext.
 * @param out  The ciphertext
 * @param size how much to encrypt
 * @param tfdata: Pointer to the global data structure containing session keys.
 * @return none
 */
void _TwoFish_BlockCrypt(u_int8_t *in,u_int8_t *out,unsigned long size,int decrypt,TWOFISH *tfdata)
{
    u_int8_t PnMinusOne[TwoFish_BLOCK_SIZE];
    u_int8_t CnMinusOne[TwoFish_BLOCK_SIZE];
    u_int8_t CBCplusCprime[TwoFish_BLOCK_SIZE];
    u_int8_t Pn[TwoFish_BLOCK_SIZE];
    u_int8_t *p,*pout;
    unsigned long i;

    /* here is where we implement CBC mode and cipher block stealing */
    if(size==TwoFish_BLOCK_SIZE)
        {
            /* if we are encrypting, CBC means we XOR the plain text block with the */
            /* previous cipher text block before encrypting */
            if(!decrypt && tfdata->qBlockDefined)
                {
                    for(p=in,i=0; i<TwoFish_BLOCK_SIZE; i++,p++)
                        Pn[i]=*p ^ tfdata->qBlockCrypt[i];	/* FK: I'm copying the xor'ed input into Pn... */
                }
            else
                memcpy(Pn,in,TwoFish_BLOCK_SIZE); /* FK: same here. we work of Pn all the time. */

            /* TwoFish block level encryption or decryption */
            _TwoFish_BlockCrypt16(Pn,out,decrypt,tfdata);

            /* if we are decrypting, CBC means we XOR the result of the decryption */
            /* with the previous cipher text block to get the resulting plain text */
            if(decrypt && tfdata->qBlockDefined)
                {
                    for (p=out,i=0; i<TwoFish_BLOCK_SIZE; i++,p++)
                        *p^=tfdata->qBlockPlain[i];
                }

            /* save the input and output blocks, since CBC needs these for XOR */
            /* operations */
            _TwoFish_qBlockPush(Pn,out,tfdata);
        }
    else
        {
            /* cipher block stealing, we are at Pn, */
            /* but since Cn-1 must now be replaced with CnC' */
            /* we pop it off, and recalculate Cn-1 */

            if(decrypt)
                {
                    /* We are on an odd block, and had to do cipher block stealing, */
                    /* so the PnMinusOne has to be derived differently. */

                    /* First we decrypt it into CBC and C' */
                    _TwoFish_qBlockPop(CnMinusOne,PnMinusOne,tfdata);
                    _TwoFish_BlockCrypt16(CnMinusOne,CBCplusCprime,decrypt,tfdata);

                    /* we then xor the first few bytes with the "in" bytes (Cn) */
                    /* to recover Pn, which we put in out */
                    for(p=in,pout=out,i=0; i<size; i++,p++,pout++)
                        *pout=*p ^ CBCplusCprime[i];

                    /* We now recover the original CnMinusOne, which consists of */
                    /* the first "size" bytes of "in" data, followed by the */
                    /* "Cprime" portion of CBCplusCprime */
                    for(p=in,i=0; i<size; i++,p++)
                        CnMinusOne[i]=*p;
                    for(; i<TwoFish_BLOCK_SIZE; i++)
                        CnMinusOne[i]=CBCplusCprime[i];

                    /* we now decrypt CnMinusOne to get PnMinusOne xored with Cn-2 */
                    _TwoFish_BlockCrypt16(CnMinusOne,PnMinusOne,decrypt,tfdata);

                    for(i=0; i<TwoFish_BLOCK_SIZE; i++)
                        PnMinusOne[i]=PnMinusOne[i] ^ tfdata->prevCipher[i];

                    /* So at this point, out has PnMinusOne */
                    _TwoFish_qBlockPush(CnMinusOne,PnMinusOne,tfdata);
                    _TwoFish_FlushOutput(tfdata->qBlockCrypt,TwoFish_BLOCK_SIZE,tfdata);
                    _TwoFish_FlushOutput(out,size,tfdata);
                }
            else
                {
                    _TwoFish_qBlockPop(PnMinusOne,CnMinusOne,tfdata);
                    memset(Pn,0,TwoFish_BLOCK_SIZE);
                    memcpy(Pn,in,size);
                    for(i=0; i<TwoFish_BLOCK_SIZE; i++)
                        Pn[i]^=CnMinusOne[i];
                    _TwoFish_BlockCrypt16(Pn,out,decrypt,tfdata);
                    _TwoFish_qBlockPush(Pn,out,tfdata);  /* now we officially have Cn-1 */
                    _TwoFish_FlushOutput(tfdata->qBlockCrypt,TwoFish_BLOCK_SIZE,tfdata);
                    _TwoFish_FlushOutput(CnMinusOne,size,tfdata);  /* old Cn-1 becomes new partial Cn */
                }
            tfdata->qBlockDefined=false;
        }
}

void _TwoFish_qBlockPush(u_int8_t *p,u_int8_t *c,TWOFISH *tfdata)
{
    if(tfdata->qBlockDefined)
        _TwoFish_FlushOutput(tfdata->qBlockCrypt,TwoFish_BLOCK_SIZE,tfdata);
    memcpy(tfdata->prevCipher,tfdata->qBlockPlain,TwoFish_BLOCK_SIZE);
    memcpy(tfdata->qBlockPlain,p,TwoFish_BLOCK_SIZE);
    memcpy(tfdata->qBlockCrypt,c,TwoFish_BLOCK_SIZE);
    tfdata->qBlockDefined=true;
}

void _TwoFish_qBlockPop(u_int8_t *p,u_int8_t *c,TWOFISH *tfdata)
{
    memcpy(p,tfdata->qBlockPlain,TwoFish_BLOCK_SIZE );
    memcpy(c,tfdata->qBlockCrypt,TwoFish_BLOCK_SIZE );
    tfdata->qBlockDefined=false;
}

/* Reset's the CBC flag and zero's PrevCipher (through qBlockPlain) (important) */
void _TwoFish_ResetCBC(TWOFISH *tfdata)
{
    tfdata->qBlockDefined=false;
    memset(tfdata->qBlockPlain,0,TwoFish_BLOCK_SIZE);
}

void _TwoFish_FlushOutput(u_int8_t *b,unsigned long len,TWOFISH *tfdata)
{
    unsigned long i;

    for(i=0; i<len && !tfdata->dontflush; i++)
        *tfdata->output++ = *b++;
    tfdata->dontflush=false;
}

void _TwoFish_BlockCrypt16(u_int8_t *in,u_int8_t *out,bool decrypt,TWOFISH *tfdata)
{
    u_int32_t x0,x1,x2,x3;
    u_int32_t k,t0,t1,R;


    x0=*in++;
    x0|=(*in++ << 8 );
    x0|=(*in++ << 16);
    x0|=(*in++ << 24);
    x1=*in++;
    x1|=(*in++ << 8 );
    x1|=(*in++ << 16);
    x1|=(*in++ << 24);
    x2=*in++;
    x2|=(*in++ << 8 );
    x2|=(*in++ << 16);
    x2|=(*in++ << 24);
    x3=*in++;
    x3|=(*in++ << 8 );
    x3|=(*in++ << 16);
    x3|=(*in++ << 24);

    if(decrypt)
        {
            x0 ^= tfdata->subKeys[4];	/* swap input and output whitening keys when decrypting */
            x1 ^= tfdata->subKeys[5];
            x2 ^= tfdata->subKeys[6];
            x3 ^= tfdata->subKeys[7];

            k = 7+(TwoFish_ROUNDS*2);
            for (R = 0; R < TwoFish_ROUNDS; R += 2)
                {
                    t0 = _TwoFish_Fe320( tfdata->sBox, x0);
                    t1 = _TwoFish_Fe323( tfdata->sBox, x1);
                    x3 ^= t0 + (t1<<1) + tfdata->subKeys[k--];
                    x3  = x3 >> 1 | x3 << 31;
                    x2  = x2 << 1 | x2 >> 31;
                    x2 ^= t0 + t1 + tfdata->subKeys[k--];

                    t0 = _TwoFish_Fe320( tfdata->sBox, x2);
                    t1 = _TwoFish_Fe323( tfdata->sBox, x3);
                    x1 ^= t0 + (t1<<1) + tfdata->subKeys[k--];
                    x1  = x1 >> 1 | x1 << 31;
                    x0  = x0 << 1 | x0 >> 31;
                    x0 ^= t0 + t1 + tfdata->subKeys[k--];
                }

            x2 ^= tfdata->subKeys[0];
            x3 ^= tfdata->subKeys[1];
            x0 ^= tfdata->subKeys[2];
            x1 ^= tfdata->subKeys[3];
        }
    else
        {
            x0 ^= tfdata->subKeys[0];
            x1 ^= tfdata->subKeys[1];
            x2 ^= tfdata->subKeys[2];
            x3 ^= tfdata->subKeys[3];

            k = 8;
            for (R = 0; R < TwoFish_ROUNDS; R += 2)
                {
                    t0 = _TwoFish_Fe320( tfdata->sBox, x0);
                    t1 = _TwoFish_Fe323( tfdata->sBox, x1);
                    x2 ^= t0 + t1 + tfdata->subKeys[k++];
                    x2  = x2 >> 1 | x2 << 31;
                    x3  = x3 << 1 | x3 >> 31;
                    x3 ^= t0 + (t1<<1) + tfdata->subKeys[k++];

                    t0 = _TwoFish_Fe320( tfdata->sBox, x2);
                    t1 = _TwoFish_Fe323( tfdata->sBox, x3);
                    x0 ^= t0 + t1 + tfdata->subKeys[k++];
                    x0  = x0 >> 1 | x0 << 31;
                    x1  = x1 << 1 | x1 >> 31;
                    x1 ^= t0 + (t1<<1) + tfdata->subKeys[k++];
                }

            x2 ^= tfdata->subKeys[4];
            x3 ^= tfdata->subKeys[5];
            x0 ^= tfdata->subKeys[6];
            x1 ^= tfdata->subKeys[7];
        }

    *out++ = (u_int8_t)(x2      );
    *out++ = (u_int8_t)(x2 >>  8);
    *out++ = (u_int8_t)(x2 >> 16);
    *out++ = (u_int8_t)(x2 >> 24);

    *out++ = (u_int8_t)(x3      );
    *out++ = (u_int8_t)(x3 >>  8);
    *out++ = (u_int8_t)(x3 >> 16);
    *out++ = (u_int8_t)(x3 >> 24);

    *out++ = (u_int8_t)(x0      );
    *out++ = (u_int8_t)(x0 >>  8);
    *out++ = (u_int8_t)(x0 >> 16);
    *out++ = (u_int8_t)(x0 >> 24);

    *out++ = (u_int8_t)(x1      );
    *out++ = (u_int8_t)(x1 >>  8);
    *out++ = (u_int8_t)(x1 >> 16);
    *out++ = (u_int8_t)(x1 >> 24);
}

/**
 * Use (12, 8) Reed-Solomon code over GF(256) to produce a key S-box
 * 32-bit entity from two key material 32-bit entities.
 *
 * @param  k0  1st 32-bit entity.
 * @param  k1  2nd 32-bit entity.
 * @return  Remainder polynomial generated using RS code
 */
u_int32_t _TwoFish_RS_MDS_Encode(u_int32_t k0,u_int32_t k1)
{
    u_int32_t i,r;

    for(r=k1,i=0; i<4; i++) /* shift 1 byte at a time */
        TwoFish_RS_rem(r);
    r ^= k0;
    for(i=0; i<4; i++)
        TwoFish_RS_rem(r);

    return r;
}

u_int32_t _TwoFish_F32(u_int32_t k64Cnt,u_int32_t x,u_int32_t *k32)
{
    u_int8_t b0,b1,b2,b3;
    u_int32_t k0,k1,k2,k3,result = 0;

    b0=TwoFish_b0(x);
    b1=TwoFish_b1(x);
    b2=TwoFish_b2(x);
    b3=TwoFish_b3(x);
    k0=k32[0];
    k1=k32[1];
    k2=k32[2];
    k3=k32[3];

    switch (k64Cnt & 3)
        {
        case 1:	/* 64-bit keys */
            result =
                TwoFish_MDS[0][(TwoFish_P[TwoFish_P_01][b0] & 0xFF) ^ TwoFish_b0(k0)] ^
                TwoFish_MDS[1][(TwoFish_P[TwoFish_P_11][b1] & 0xFF) ^ TwoFish_b1(k0)] ^
                TwoFish_MDS[2][(TwoFish_P[TwoFish_P_21][b2] & 0xFF) ^ TwoFish_b2(k0)] ^
                TwoFish_MDS[3][(TwoFish_P[TwoFish_P_31][b3] & 0xFF) ^ TwoFish_b3(k0)];
            break;
        case 0:	/* 256-bit keys (same as 4) */
            b0 = (TwoFish_P[TwoFish_P_04][b0] & 0xFF) ^ TwoFish_b0(k3);
            b1 = (TwoFish_P[TwoFish_P_14][b1] & 0xFF) ^ TwoFish_b1(k3);
            b2 = (TwoFish_P[TwoFish_P_24][b2] & 0xFF) ^ TwoFish_b2(k3);
            b3 = (TwoFish_P[TwoFish_P_34][b3] & 0xFF) ^ TwoFish_b3(k3);

        case 3:	/* 192-bit keys */
            b0 = (TwoFish_P[TwoFish_P_03][b0] & 0xFF) ^ TwoFish_b0(k2);
            b1 = (TwoFish_P[TwoFish_P_13][b1] & 0xFF) ^ TwoFish_b1(k2);
            b2 = (TwoFish_P[TwoFish_P_23][b2] & 0xFF) ^ TwoFish_b2(k2);
            b3 = (TwoFish_P[TwoFish_P_33][b3] & 0xFF) ^ TwoFish_b3(k2);
        case 2:	/* 128-bit keys (optimize for this case) */
            result =
                TwoFish_MDS[0][(TwoFish_P[TwoFish_P_01][(TwoFish_P[TwoFish_P_02][b0] & 0xFF) ^ TwoFish_b0(k1)] & 0xFF) ^ TwoFish_b0(k0)] ^
                TwoFish_MDS[1][(TwoFish_P[TwoFish_P_11][(TwoFish_P[TwoFish_P_12][b1] & 0xFF) ^ TwoFish_b1(k1)] & 0xFF) ^ TwoFish_b1(k0)] ^
                TwoFish_MDS[2][(TwoFish_P[TwoFish_P_21][(TwoFish_P[TwoFish_P_22][b2] & 0xFF) ^ TwoFish_b2(k1)] & 0xFF) ^ TwoFish_b2(k0)] ^
                TwoFish_MDS[3][(TwoFish_P[TwoFish_P_31][(TwoFish_P[TwoFish_P_32][b3] & 0xFF) ^ TwoFish_b3(k1)] & 0xFF) ^ TwoFish_b3(k0)];
            break;
        }
    return result;
}

u_int32_t _TwoFish_Fe320(u_int32_t *lsBox,u_int32_t x)
{
    return lsBox[        TwoFish_b0(x)<<1    ]^
           lsBox[      ((TwoFish_b1(x)<<1)|1)]^
           lsBox[0x200+ (TwoFish_b2(x)<<1)   ]^
           lsBox[0x200+((TwoFish_b3(x)<<1)|1)];
}

u_int32_t _TwoFish_Fe323(u_int32_t *lsBox,u_int32_t x)
{
    return lsBox[       (TwoFish_b3(x)<<1)   ]^
           lsBox[      ((TwoFish_b0(x)<<1)|1)]^
           lsBox[0x200+ (TwoFish_b1(x)<<1)   ]^
           lsBox[0x200+((TwoFish_b2(x)<<1)|1)];
}

u_int32_t _TwoFish_Fe32(u_int32_t *lsBox,u_int32_t x,u_int32_t R)
{
    return lsBox[      2*TwoFish__b(x,R  )  ]^
           lsBox[      2*TwoFish__b(x,R+1)+1]^
           lsBox[0x200+2*TwoFish__b(x,R+2)  ]^
           lsBox[0x200+2*TwoFish__b(x,R+3)+1];
}


#endif
