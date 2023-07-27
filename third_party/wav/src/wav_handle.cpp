#include "wav_handle.h"

CWavHandle::CWavHandle()
{
}

CWavHandle::~CWavHandle()
{
	CloseFlie();
}

int32_t CWavHandle::OpenWavFile(const std::string& fileName)
{
	if (file)
	{
		AS_LOG(AS_LOG_WARNING,"The file fas opened.");
		return AS_ERROR_CODE_OK;
	}
	WaveRIFF riff;
	WaveFormat format;
	WaveData data;
	int32_t userDataSize;
	int errNum;
	file = fopen(fileName.c_str(), "rb+");
	if (!file)
	{
		errNum = errno;
		AS_LOG(AS_LOG_WARNING,"open wav file %s fail, reason %s.", fileName.c_str(), strerror(errNum));
		return AS_ERROR_CODE_FAIL;
	}

	if (fread(&riff, 1, sizeof(riff), static_cast<FILE*>(file)) != sizeof(riff))
	{
		AS_LOG(AS_LOG_WARNING,"the header of data is incorrect and read riff fail.");
		if (fclose(static_cast<FILE*>(file)) == EOF) {
			AS_LOG(AS_LOG_WARNING,"close wav file fail.");
		}
		file = NULL;
		return AS_ERROR_CODE_FAIL;
	}

	if (std::string(riff.chunkId, 4) != "RIFF" || std::string(riff.format, 4) != "WAVE") 
	{
		AS_LOG(AS_LOG_WARNING,"The header of data is incorrect and is not wav.");
		if (fclose(static_cast<FILE*>(file)) == EOF) 
		{
			AS_LOG(AS_LOG_WARNING,"Close the file fail.");
		}
		file = NULL;
		return AS_ERROR_CODE_FAIL;
	}

	if (fread(&format, 1, sizeof(format), static_cast<FILE*>(file)) != sizeof(format)) 
	{
		AS_LOG(AS_LOG_WARNING,"The header of data is incorrect and read format fail.");
		if (fclose(static_cast<FILE*>(file)) == EOF) 
		{
			AS_LOG(AS_LOG_WARNING,"Close the file fail.");
		}
		file = NULL;
		return AS_ERROR_CODE_FAIL;
	}

	if (std::string(format.subchunk1Id, 4) != "fmt ")
	{
		AS_LOG(AS_LOG_WARNING,"The header of data is incorrect and lose fmt.");
		if (fclose(static_cast<FILE*>(file)) == EOF) {
			AS_LOG(AS_LOG_WARNING,"Close the file fail.");
		}
		file = NULL;
		return AS_ERROR_CODE_FAIL;
	}

	if (format.audioFormat != 1) 
	{
		AS_LOG(AS_LOG_WARNING,"The program does not support,the data format is not pcm and should only support pcm.");
		if (fclose(static_cast<FILE*>(file)) == EOF) {
			AS_LOG(AS_LOG_WARNING,"Close the file fail.");
		}
		file = NULL;
		return AS_ERROR_CODE_FAIL;
	}

	userDataSize = format.subchunk1Size - sizeof(format) + 8;
	if (userDataSize < 0) 
	{
		AS_LOG(AS_LOG_WARNING,"The header of data is incorrect and the blocksize is abnormal.");
		if (fclose(static_cast<FILE*>(file)) == EOF) {
			AS_LOG(AS_LOG_WARNING,"Close the file fail.");
		}
		file = NULL;
		return AS_ERROR_CODE_FAIL;
	}
	else if (userDataSize > 0) 
	{
		if (fseek(static_cast<FILE*>(file), userDataSize, SEEK_CUR) != 0)//偏移量
		{
			AS_LOG(AS_LOG_WARNING,"read wav file fail.");
			if (fclose(static_cast<FILE*>(file)) == EOF) {
				AS_LOG(AS_LOG_WARNING,"close wav file fail.");
			}
			file = NULL;
			return AS_ERROR_CODE_FAIL;
		}
	}

	while (1) 
	{
		if (fread(&data, 1, sizeof(data), static_cast<FILE*>(file)) != sizeof(data))
		{
			AS_LOG(AS_LOG_WARNING,"read wav file fail.");
			if (fclose(static_cast<FILE*>(file)) == EOF) {
				AS_LOG(AS_LOG_WARNING,"close wav file fail.");
			}
			file = NULL;
			return AS_ERROR_CODE_FAIL;
		}
		if (std::string(data.subchunk2Id, 4) != "data")
		{
			if (fseek(static_cast<FILE*>(file), data.subchunk2Size, SEEK_CUR) != 0) 
			{
				AS_LOG(AS_LOG_WARNING,"read wav file fail.");
				if (fclose(static_cast<FILE*>(file)) == EOF) {
					AS_LOG(AS_LOG_WARNING,"close wav file fail.");
				}
				file = NULL;
				return AS_ERROR_CODE_FAIL;
			}
			continue;
		}
		break;
	}
	m_ulDataOffset = ftell(static_cast<FILE*>(file));//文件指针当前位置指文件首部的字节数
	m_ulFileLength = riff.chunkSize + 8;
	m_ulDataLength = data.subchunk2Size;
	m_ulChannels = format.channels;
	m_ulSampleRate = format.sampleRate;
	m_ulBitsPerSample = format.bitsPerSample;
	AS_LOG(AS_LOG_INFO,"the dataOffset[%d] fileLength[%u] dataLength[%u] channel[%d]  sampleRate[%d] bitsPerSample[%d]",
		m_ulDataOffset,m_ulFileLength,m_ulDataLength,m_ulChannels,m_ulSampleRate,m_ulBitsPerSample);
	return AS_ERROR_CODE_OK;
}

void CWavHandle::CloseFlie()
{
	if (file) {
		if (fclose(static_cast<FILE*>(file)) == EOF) {
			AS_LOG(AS_LOG_WARNING,"close the file fail.");
		}
		file = NULL;
	}
}

int32_t CWavHandle::ReadData(unsigned char* buf, int32_t bufLength)
{
	if (ftell(static_cast<FILE*>(file)) >= m_ulDataOffset + m_ulDataLength) {
		return 0;
	}
	return fread(buf, 1, bufLength, static_cast<FILE*>(file));
}

int32_t CWavHandle::SetPosition(int32_t postion)
{
	if (fseek(static_cast<FILE*>(file), m_ulDataOffset + postion, SEEK_SET) != 0) {
		AS_LOG(AS_LOG_WARNING,"wav locate fail.");
		return AS_ERROR_CODE_FAIL;
	}
	return 0;
}

int32_t CWavHandle::GetPosition()
{
	return ftell(static_cast<FILE*>(file)) - m_ulDataOffset;
}

int32_t CWavHandle::GetFileLength()
{
	return m_ulFileLength;
}

uint32_t CWavHandle::GetDataLength()
{
	return m_ulDataLength;
}

int32_t CWavHandle::GetChannels()
{
	return m_ulChannels;
}

int32_t CWavHandle::GetSampleRate()
{
	return m_ulSampleRate;
}

int32_t CWavHandle::GetBitsPerSample()
{
	return m_ulBitsPerSample;
}