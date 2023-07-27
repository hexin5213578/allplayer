#pragma once

unsigned char linear2alaw(short pcm_val);

unsigned char linear2ulaw(short pcm_val);

int G711EnCode(unsigned char* pCodecBits, char* pBuffer, int nBufferSize, int type);