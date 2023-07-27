#include "g711_encode.h"


#define SIGN_BIT    (0x80)      /* Sign bit for a A-law byte. */  
#define QUANT_MASK  (0xf)       /* Quantization field mask. */  
#define NSEGS       (8)     /* Number of A-law segments. */  
#define SEG_SHIFT   (4)     /* Left shift for segment number. */  
#define SEG_MASK    (0x70)      /* Segment field mask. */  
#define BIAS        (0x84)      /* Bias for linear code. */


static unsigned char ALawCompressTable[] =
{
    1, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static unsigned char MuLawCompressTable[] =
{
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

//Encode
int G711EnCode(unsigned char* pCodecBits, char* pBuffer, int nBufferSize, int type)
{
    if (pCodecBits == nullptr || pBuffer == nullptr || nBufferSize <= 0) {
        return -1;
    }
        
    unsigned char* codecbits = (unsigned char*)pCodecBits;
    short* buffer = (short*)pBuffer;

    if (type == 0) {
        for (int i = 0; i < nBufferSize / 2; i++) {
            codecbits[i] = linear2alaw(buffer[i]);
        }
    }
    else {
        for (int i = 0; i < nBufferSize / 2; i++) {
            codecbits[i] = linear2ulaw(buffer[i]);
        }
    }
    return nBufferSize / 2;
}

unsigned char linear2alaw(short sample)    /* 2's complement (16-bit range) */
{
    int sign = 0;
    int exponent = 0;
    int mantissa = 0;
    unsigned char compressedByte = 0;

    sign = ((~sample) >> 8) & 0x80;
    if (sign == 0) {
        sample = (short)-sample;
    }
    if (sample > 0x7F7B) {
        sample = 0x7F7B;
    }
    if (sample >= 0x100) {
        exponent = (int)ALawCompressTable[(sample >> 8) & 0x7F];
        mantissa = (sample >> (exponent + 3)) & 0x0F;
        compressedByte = (unsigned char)((exponent << 4) | mantissa);
    }
    else {
        compressedByte = (unsigned char)(sample >> 4);
    }
    compressedByte ^= (unsigned char)(sign ^ 0x55);
    return compressedByte;
}

unsigned char linear2ulaw(short sample)    /* 2's complement (16-bit range) */
{
    int cBias = 0x84;
    int cClip = 0x7F7B;
    int sign = (sample >> 8) & 0x80;

    if (sign != 0) {
        sample = (short)-sample;
    }
    if (sample > cClip) {
        sample = (short)cClip;
    }
    sample = (short)(sample + cBias);

    int exponent = (int)MuLawCompressTable[(sample >> 7) & 0xFF];
    int mantissa = (sample >> (exponent + 3)) & 0x0F;
    int compressedByte = ~(sign | (exponent << 4) | mantissa);
    return (unsigned char)compressedByte;
}