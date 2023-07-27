#pragma once

#include "as.h"
#include "wav_common.h"
#include <string>
#include <errno.h>

/************************************************************************
* @Decription:  wav文件读取工具
* 本版本只支持pcm读取，且未处理字节顺序。	riff文件是小端，通常在int32_tel的设备上是没问题的，在java虚拟机上则需要处理。
************************************************************************/

class CWavHandle 
{
public:
	CWavHandle();
	~CWavHandle();
	int32_t OpenWavFile(const std::string& fileName);
	void 	CloseFlie();
	int32_t ReadData(unsigned char* buf, int32_t bufLength);
	int32_t SetPosition(int32_t position);
	int32_t GetPosition();
	int32_t GetFileLength();
	uint32_t GetDataLength();
	int32_t GetChannels();
	int32_t GetSampleRate();
	int32_t GetBitsPerSample();
private:
	void*	file = NULL;
	uint32_t m_ulFileLength = 0;
	uint32_t m_ulDataLength = 0;
	int32_t  m_ulChannels = 0;
	int32_t  m_ulSampleRate = 0;
	int32_t  m_ulBitsPerSample = 0;
	int32_t  m_ulDataOffset = 0;
};

