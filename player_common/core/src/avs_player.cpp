#include "avs_player.h"
#include "avs_rtsp_ingress.h"
#include "avs_video_thread.h"
#include "avs_audio_thread.h"
#include "avs_refresh_loop.h"

void AvsPlayer::stop() {
	if (m_refreshLoop) {
		m_refreshLoop->stopRun();
		AS_DELETE(m_refreshLoop);
	}

	if (pictq) {
		pictq->signal();
	}
	
	if (m_videoThread) {
		m_videoThread->stopRun();
		AS_DELETE(m_videoThread);
	}

	if (sampq) {
		sampq->signal();
	}
	if (m_audioThread) {
		m_audioThread->stopRun();
		AS_DELETE(m_audioThread);
	}
	return;
}

void AvsPlayer::setVolume(int16_t volume) {
	if (!m_audioThread) {
		return;
	}

	if (volume > 0 && !m_audioThread->getStarted()) {
		m_audioThread->startRun();
	}
	this->audio_volume = volume;
	m_audioThread->setVolume(volume);
	return;
}

void AvsPlayer::setAudioChannel(bool start) {
	if (start) {
		setVolume(m_audioVol);
	}
	else {
		m_audioVol = this->audio_volume;
		setVolume(-1);
	}
	return;
}

void AvsPlayer::setScale(double scale) {
	if (!this->realtime) {
		this->speed = scale;
		this->vidclk.setClockSpeed(this->speed);
		this->extclk.setClockSpeed(this->speed);
	}
}

void AvsPlayer::surfaceDiff(int width, int height) {
	if (m_refreshLoop) {
		m_refreshLoop->surfaceDiff(width, height);
	}
}

int AvsPlayer::setMediaAttribute(MK_Format_Contex* format) {
	if (!format) {
		return AVERROR(EINVAL);
	}

	AVCodecID codecId = AV_CODEC_ID_NONE;
	int32_t ret = 0;
	MK_Stream* stream = nullptr;

	if ((format->video_stream >= 0) && ((void*)INVALID_WND != m_hwnd)) {
		stream = format->streams.at(format->video_stream);
		if (stream && stream->codecpar) {
			if ((ret = configVideo(stream)) < 0) {
				return ret;
			}
		}
		video_stream = format->video_stream;
	}

	if (format->audio_stream >= 0) {
		stream = format->streams.at(format->audio_stream);
		if (stream && stream->codecpar) {
			if ((ret = configAudio(stream)) < 0)
				return ret;
		}
		audio_stream = format->audio_stream;
	}
	return ret;
}

int AvsPlayer::adaptMediaAttribute(MK_Format_Contex* fmt) {
	//XXX: video decoder adapt
	int ret = 0;
	MK_Stream* stream = nullptr;
	if (m_audioThread && fmt->audio_stream >= 0) {
		stream = fmt->streams.at(fmt->audio_stream);
		if (stream) {
			ret = m_audioThread->openSoftDecoder(stream);
		}
	}
	return ret;
}

int AvsPlayer::adjustPictureParams(std::string pic) {
	if (m_videoThread) {
		return m_videoThread->adjustPictureParams(pic);
	}
	return AVERROR(EINVAL);;
}

std::string AvsPlayer::getPictureParams() {
	if (m_videoThread) {
		return m_videoThread->getPictureParams();
	}
	return "";
}

int AvsPlayer::snapPicture(std::string filePath, uint8_t format) {
	if (m_videoThread) {
		return m_videoThread->snapPicture(filePath, format) ? 0 : 1;
	}
	return AVERROR(EINVAL);;
}

void AvsPlayer::captureFrames(std::string capturePath, uint16_t count) {
	if (m_videoThread) {
		m_videoThread->captureFrames(capturePath, count);
	}
}

void AvsPlayer::setSkipNokey(bool skip) {
	if (!m_videoThread) {
		return;
	}
	//m_bSkipNokeyFrame = skip;
	AS_LOG(AS_LOG_INFO, "player[%ld] set skip non-key frame %s.", this->id, skip ? "on" : "off");
	m_videoThread->skipNonKey(skip);
}

int AvsPlayer::zoomOut(ZoomSt& zoom) {
	if (!m_refreshLoop) {
		return AVERROR(EINVAL);
	}
	return m_refreshLoop->zoomOut(zoom.zoomHwnd, zoom.top, zoom.right, zoom.bottom, zoom.left);
}

int AvsPlayer::zoomIn() {
	if (!m_refreshLoop) {
		return AVERROR(EINVAL);
	}
	return m_refreshLoop->zoonIn();
}

void AvsPlayer::getStallingInfo(int& stallEvent, double& stallaDuration) {
	if (m_refreshLoop) {
		m_refreshLoop->getStallingInfo(stallEvent, stallaDuration);
	}
	else {
		stallEvent = 0;
		stallaDuration = 0.0;
	}
}

int AvsPlayer::getFps() {
	if (m_refreshLoop) {
		return m_refreshLoop->getFps();
	}
	return 0;
}

void AvsPlayer::clearBuffer() {
	if (m_videoThread) {
		m_videoThread->clear();
	}
	if (m_audioThread) {
		m_audioThread->clear();
	}
}

void AvsPlayer::clearSurface()
{

}

void AvsPlayer::changeSurface(void* win, int width, int height)
{
	if (m_refreshLoop) {
		m_refreshLoop->setWindow(win);
		m_refreshLoop->surfaceDiff(width, height);
	}
} 

//void AvsPlayer::addObserver(IObserver* observer) {
//	if (m_refreshLoop) {
//		m_refreshLoop->addObserver(observer);
//	}
//}

