#pragma once

#include <list>

#include "libMediaService.h"
#include "avs_thread_base.h"
#include "avs_video_state.h"

std::string ffmpeg_err(int errnum);

class AvsDecodeThread : public AvsThreadBase, public Task
{
public:
	AvsDecodeThread(VideoState* ic);
	virtual ~AvsDecodeThread();

	int openSoftDecoder(AVStream* stream);
	int openSoftDecoder(MK_Stream* stream);
	void closeDecoder();

	//清理缓存队列与解码器
	virtual void clear();
	void seek();
	virtual void pause(bool isPause, bool needClear = false);
	void stopRun() override;
	virtual void restart();

	void skipNonKey(bool skip);

	void threshold(int64_t threshold) {	m_threshold = threshold; }
	
	int getPktqSize() {	return m_pktq->packetSize(); }

	int enoughPkts() { return m_pktq->getAbort() || m_pktq->packetSize() > 5; }

protected:
	virtual void mainProcess() override;
	virtual int decoding() = 0;

	int decodeFrame();

	virtual int processFrame(AVFrame* frame) = 0;
	virtual void pushPacket(AVPacket* pkt) = 0;

protected:
	VideoState*					m_videoState;
	MK_CodecParameters*			m_codecParameters = nullptr;
	
	AVPacket					m_pendPkt;					//EAGAIN导致pending的数据包	
	int							m_pktPending = 0;		

	int64_t						m_duration = 0;
	AvsDecoder*					m_decoder = nullptr;
	PacketQueue::Ptr			m_pktq;
	bool						m_needDisplay = true;				//是否需要显示
	AVFrame*					m_frame = nullptr;					//解码后存储 
	std::mutex					m_ctxMutex;						//上下文等资源锁
	bool						m_hardWareAcc = false;
	int64_t						m_threshold = 0;

	int8_t						m_stepDirect = 0;
	int							m_serial = -1;
	int							m_finished = 0;
	volatile bool				m_arriveEOF = false;
	volatile bool				m_skipNonKey = false;
};

