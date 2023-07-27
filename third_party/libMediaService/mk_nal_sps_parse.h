#ifndef _AS_NAL_SPS_PARSE_H_
#define _AS_NAL_SPS_PARSE_H_


int h264_decode_sps(char * buf, unsigned int nLen, int &width, int &height, int &fps);

int h265_decode_sps(char * buf, unsigned int nLen, int &width, int &height, int &fps);

void h264_parse_sps(const unsigned char* pStart, unsigned short nLen,int& width,int& height,int& fps);

#endif