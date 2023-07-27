/*****************************************************************//**
 * \file   avs_decoder.h
 * \brief  FFmpeg解码类
 * 
 * \author songgan
 * \date   April 2021
 *********************************************************************/
#pragma once

#include <mutex>

#include "libMediaService.h"
#include "util.h"

extern "C" {
#include "libavcodec/avcodec.h"
}

int CodecParasCopy(AVCodecParameters* dst, const MK_CodecParameters* src);

class AvsDecoder
{
public:
	AvsDecoder() = default;
	virtual ~AvsDecoder();
	
	int	probeHwAcc(AVCodecID codecId);
	int initFF(AVStream* stream, bool bHwAcc = false);
	int initMK(MK_Stream* stream, bool hw = false);		//创建编解码器

	int getCodecId() {
		return m_codecId;
	}

	//发送解码pkt
	int sendPkt(const AVPacket* pkt);

	//获取解码帧
	bool recvFrame(AVFrame* frame);

	int recvFrameInt(AVFrame* frame);

	void flushCodecBuffer();

	//将frame保存为jpeg
	bool saveFrameAsJpeg(AVFrame* pFrame, std::string filePath);

	bool saveFrameAsBMP(AVFrame* pFrame, std::string filePath);

	AVPixelFormat getHWPixelFormat() { 
		return m_hWPixelFmt;
	}

	void clear(bool close = false);

	void close();

	bool checkY(int iwidth, int iHeight, unsigned char*Buf);

	AVFrame* transferFrameData(AVFrame* hwframe, uint8_t** src);	//transfer hw frame to memory 

protected:

	int						m_codecId = 0;
	AVCodecContext*			m_codecCtx = nullptr;
	//编解码上下文锁
	std::mutex				m_ctxMtx;
	AVFrame*				m_frameRGB = nullptr;
	AVBufferRef*			m_hwCtx = nullptr;
	AVPixelFormat			m_hWPixelFmt = AVPixelFormat::AV_PIX_FMT_NONE;
	AVStream*				m_avStream = nullptr;
	MK_Stream*				m_mkStream = nullptr;
};

