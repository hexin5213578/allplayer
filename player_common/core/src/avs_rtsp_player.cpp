#include "avs_rtsp_player.h"
#include "avs_video_thread.h"
#include "avs_audio_thread.h"
#include "avs_refresh_loop.h"
#include "avs_mux_thread.h"
#include "avs_audio_play.h"
#include "view_factory.h"

#if ((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include "avs_ios_refresh_loop.h"
#include "ios_audio_thread.h"
#endif

void AvsRtspBasePlayer::pause(bool pause, bool clear) {
	if (this->paused == pause) {
		return;
	}

	if (this->paused) {
		this->frame_timer += getRelativeTime() - this->vidclk.last_updated;
		this->vidclk.paused = 0;
		this->vidclk.setClock(this->vidclk.getClock(), this->vidclk.serial);
	}
	this->extclk.setClock(this->extclk.getClock(), this->extclk.serial);
	this->paused = this->audclk.paused = this->vidclk.paused = this->extclk.paused = !this->paused;

	if (m_videoThread) {
		m_videoThread->pause(this->paused, clear);
	}

	if (m_audioThread) {
		m_audioThread->pause(this->paused, clear);
	}
}

AvsRtspPlayer::AvsRtspPlayer(BizParams& bizParams) : AvsRtspBasePlayer(bizParams) {
	this->play_mode = kRtsp;

	if (TYPE_REALVIDEO_START == this->type || 
		TYPE_MULTI_REALVIDEO_START == this->type) {
		this->realtime = 1;
		auto cache = bizParams.CacheSize;
		if (cache <= 0) {
			this->pre_cache = 1;
		}
		else if (cache > 50) {
			this->pre_cache = 50;
		}
		else {
			this->pre_cache = cache;
		}
	}
}

int AvsRtspPlayer::play() {
	int ret = AvsRtspProcessor::init_1(this);
	if (ret < 0) {
		return ret;
	}
	ret = start_1();
	if (ret < 0) {
		return ret;
	}
	return AvsRtspBasePlayer::play();
}

int AvsRtspBasePlayer::configVideo(MK_Stream* stream) {
	int32_t ret = 0;
	MK_CodecParameters* codecpar = stream->codecpar;
	bool setUrl = !!m_videoThread;

	do {
		if (!m_videoThread) {
			m_videoThread = new AvsVideoThread(this);
			if (!m_videoThread) {
				ret = AVERROR(ENOMEM);
				break;
			}
		}
		
		if (m_hardwareAcc) {
			ret = m_videoThread->probeHwAcc(stream);
		}
	
		if (!m_hardwareAcc || 0 > ret)	{ 	//fallback to soft decode
			if (ret = m_videoThread->openSoftDecoder(stream)) {  //fail
				break;
			}
			else {
				if (m_hardwareAcc) {
					AS_LOG(AS_LOG_WARNING, "player[%ld] hardacc fail, fallback to soft.", this->id);
				}
				m_hardwareAcc = false;
			}
		}
		if (!m_refreshLoop) {
#if((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
			m_refreshLoop = new AvsIosRefreshLoop(this);
#else
			m_refreshLoop = new AvsRefreshLoop(this);
#endif
			if (!m_refreshLoop) {
				ret = AVERROR(ENOMEM);
				break;
			}
		}
		ret = m_refreshLoop->setView(m_view);
        m_refreshLoop->setWindow(m_hwnd);
		if (ret < 0) {
			break;
		}
	} while (0);

	if (ret < 0) {
		if (m_videoThread) {
			m_videoThread->stopRun();
			AS_DELETE(m_videoThread);
		}
		return ret;
	}

	//reset url when video thread in quality mode
	if (setUrl) {
		while (this->pictq->remainingNb() > 0) {
			this->pictq->queueNext();
		}
		m_videoThread->restart();
	}

	if (!m_videoThread->getStarted()) {
		m_videoThread->startRun();
	}

	if (!m_refreshLoop->getStarted()) {
		m_refreshLoop->startRun();
	}
	return ret;
}

int AvsRtspBasePlayer::configAudio(MK_Stream* stream) {
	int32_t ret = 0;
	MK_CodecParameters* codecpar = stream->codecpar;
	AVCodecID codecId = (AVCodecID)codecpar->codec_id;

	bool setUrl = !!m_audioThread;

	do {
		if (!m_audioThread) {
#if ((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
            m_audioThread = new IOSAudioThread(this);
#else
			m_audioThread = new AvsAudioThread(this);
#endif
			if (!m_audioThread) {
				ret = AVERROR(ENOMEM);
				break;
			}
		}

		if (ret = m_audioThread->openSoftDecoder(stream)) {
			break;
		}

#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
		m_audioThread->initAudioPara(codecpar, audio_volume);
#endif

	} while (0);

	if (ret < 0) {
		if (m_audioThread) {
			m_audioThread->stopRun();
			AS_DELETE(m_audioThread);
		}
		return ret;
	}
	//reset url when video thread in quality mode
	if (setUrl) {
		m_audioThread->restart();
	}

	if (!m_audioThread->getStarted()) {
		m_audioThread->startRun();
	}
	return ret;
}

int AvsRtspBasePlayer::doTask(AVPacket* pkt) {
	int ret = 0;
	if (pkt->stream_index == video_stream) {
		if (m_videoThread) {
			ret = m_videoThread->doTask(pkt);
			if (ret > 0) {
				full_cond.notify_all();
			}
		}
	}
	else if (pkt->stream_index == audio_stream) {
		if (m_audioThread) {
			ret = m_audioThread->doTask(pkt);
		}
	}
	else {
		ret = AVERROR(EINVAL);
	}
	return ret;
}

void AvsRtspPlayer::startCapture(BizParams capture) {
	std::unique_lock<std::mutex> lck(m_recordMutex);
	if (m_recording || !m_formatCtx) {
		return;
	}

	capture.BizId = this->id;
	capture.BizType = this->type;
	m_recordTh = new AvsMuxThread(capture);
	if (m_recordTh) {
		m_recordTh->setMediaAttribute(m_formatCtx);
		addDelegate(m_recordTh);
		m_recordTh->play();
		m_recording = true;
	}
	else {
		AS_DELETE(m_recordTh);
	}
	return;
}

void AvsRtspPlayer::stopCapture() {
	std::unique_lock<std::mutex> lck(m_recordMutex);
	if (!m_recording || !m_recordTh) {
		return;
	}
	removeDelegate(m_recordTh);
	AS_DELETE(m_recordTh);
	m_recording = false;
}

void AvsRtspPlayer::step2NextFrame(uint8_t forward) {
	int8_t direct = forward ? forward : -1;
	step2NextFrame_1(direct);
	step2NextFrame_2(direct);
}

void AvsRtspPlayer::exitStep() {
	exitStep_1();
	exitStep_2(false);
}

void AvsRtspPlayer::exitStep_2(bool isSeek) {
	if (m_videoThread) {
		m_videoThread->exitStep(isSeek);
	}
	
	if (this->paused) {		//恢复播放状态
		pause(false, isSeek);
	}
}

void AvsRtspPlayer::step2NextFrame_2(int8_t direct) {
	if (this->paused) {
		//xx: refresh单帧, video暂停 
		//pause(false, false);
	}
}

int AvsRtspPlayer::setUrl(BizParams params) {
	void* hwnd = m_hwnd;
	m_bizParams = params;
	m_bizParams.BizType = this->type;
	m_bizParams.WindowsHandle = hwnd;
	
	int ret = reload_1(m_bizParams);
	if (ret) {
		return ret;
	}
	return 0;
}

void AvsRtspPlayer::pause(bool pause, bool clear) {
	if (this->paused == pause) {
		return;
	}

	if (this->paused) {
		this->resume_1();
	}
	else {
		this->pause_1();
	}

	if (this->paused) {
		this->frame_timer += getRelativeTime() - this->vidclk.last_updated;
		this->vidclk.paused = 0;
		this->vidclk.setClock(this->vidclk.getClock(), this->vidclk.serial);
	}
	this->extclk.setClock(this->extclk.getClock(), this->extclk.serial);
	this->paused = this->audclk.paused = this->vidclk.paused = this->extclk.paused = !this->paused;

	if (m_videoThread) {
		m_videoThread->pause(this->paused, clear);
	}
	
	if (m_audioThread) {
		m_audioThread->pause(this->paused, clear);
	}
}

int AvsRtspPlayer::vcrControl(double start, double scale, int option) {
	auto ret = vcrControl_1(start, scale, option);
	if (ret < 0) {
		return ret;
	}
	this->speed = mapSpeed(scale);
	//exit step and flush packet
	if (start >= 0.0) {
		exitStep_2(true);
	} else {
		pause(false);
	}
	setScale(this->speed);
	return 0;
}
