#ifndef __MAV_COMMON_H__
#define __MAV_COMMON_H__

#include "as.h"

#define IP_LEN                64
#define MAV_TEST              0x37
#define SVR_VERSION_INFO      "wav v1.0"

static  const char* const strDescription = "wav_test";

//WAV文件头部信息
#pragma pack(push,1)
struct WaveRIFF 
{
	const	char chunkId[4] = { 'R','I', 'F', 'F' };//RIFF标准头部标识
	uint32_t chunkSize;//标识后面整个编码的大小   4 + (8 + subchunk1Size) + (8 + subchunk2Size)
	const	char format[4] = { 'W','A', 'V', 'E' };//WAVE标准头部标识
};

struct WaveFormat
{
	const	char subchunk1Id[4] = { 'f','m', 't', ' ' };//可以为fmt或者chunk
	uint32_t subchunk1Size = 16;
	uint16_t audioFormat;//pcm=1(线性量化)
	uint16_t channels;//声道数
	uint32_t sampleRate;//采样率
	uint32_t byteRate;//sampleRate*channels*bitsPerSample/8
	uint16_t blockAlign;
	uint16_t  bitsPerSample;//每个采样点的对应位数
};

struct  WaveData
{
	const	char subchunk2Id[4] = { 'd','a', 't', 'a' };//“data”标志位
	uint32_t subchunk2Size;//pcm音频数据的长度字节数
};
#pragma pack(pop)


#endif