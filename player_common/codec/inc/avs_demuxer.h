/*****************************************************************//**
 * \file   avs_demux.h
 * \brief  解封装类 
 * 
 * \author sg
 * \date   September 2021
 *********************************************************************/

#pragma once

#include <memory>
#include "avs_video_state.h"

extern "C"
{
#include <libavutil/opt.h>
}

enum MEDIA {
	VIDEO = 0,
	AUDIO = 1,
	UNKOWN = 2
};

class AvsDemuxer
{
public:
	AvsDemuxer(VideoState* ic);
	virtual ~AvsDemuxer();

	static std::string probeUrl(const std::string& url);

	int openInput(const std::string &url);
	void close();

	int read(AVPacket* pkt);
	int seek(double pos,MEDIA type);
	
protected:
	VideoState*				m_videoState;
	AVFormatContext*		m_pFmtCtx;
	std::mutex				m_ctxMtx;
};

