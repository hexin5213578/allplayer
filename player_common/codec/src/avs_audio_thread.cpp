#include "avs_audio_thread.h"

#include "avs_player_common.h"
#include "as_config.h"
#include "avs_audio_filter.h"
#include "avs_resample.h"

AvsAudioThread::AvsAudioThread(VideoState* ic)
	:AvsDecodeThread(ic),
	m_volume(ic->audio_volume),
	m_filter(nullptr),
	m_audioPlayer(nullptr),
	m_resample(nullptr)
{
	m_needDisplay = (m_volume > 0);
	if (m_volume >= 0 && kNormal != ic->play_mode) {
#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
		AvsAudioPlay::GetInstance()->setVolume(m_volume);
#endif
	}		

	if (!ic->audioq) {
		ic->audioq = make_shared<PacketQueue>();
	}

	m_pktq = ic->audioq;
	if (!ic->sampq) {
		ic->sampq = make_shared<FrameQueue>(m_pktq, 90);
	}
	ic->audclk.init(&ic->audioq->m_serial);
	m_pktq->start();
}

int AvsAudioThread::doTask(AVPacket* pkt)
{
	int ret = -1;
	static double eps = 0.0001;
	if (fabs(m_videoState->speed - 1.0) > eps) {
		return -1;
	}
	if (m_videoState->paused || getPaused() || !m_needDisplay) {
		return 0;
	}
	m_pktq->packetPut(pkt);
	return 0;
}

void AvsAudioThread::pushPacket(AVPacket* pkt)
{
	if (pkt) {	 //if null, flush stream
		m_pktq->packetPut(pkt);
	}
	else {
		m_pktq->packetqFlush();
	}
}

void AvsAudioThread::stopRun()
{
	AvsDecodeThread::stopRun();
	AvsAudioPlay::DestroyInstance(m_audioPlayer);
	AS_DELETE(m_filter);
}

int AvsAudioThread::decoding() 
{
	int ret = 0;
	int got_frame = 0;
	double time = 0.0, pts, duration;

	while (!m_bIsExit) {
		if (!m_videoState->cached_enough) {
			std::unique_lock<decltype(m_videoState->full_mutex)> lck(m_videoState->full_mutex);
			m_videoState->full_cond.wait_for(lck, std::chrono::milliseconds(10));
			continue;
		}

		if (getPaused()) {
			AVSleep(0.01);
			continue;
		}

		if ((got_frame = decodeFrame()) < 0) {
			break;
		}

		if (!got_frame) {
			if (m_finished == m_serial) {
				m_arriveEOF = false;
			}
	
			AVSleep(0.01);
			continue;
		}

		/*
		MyFrame* af;
		if (m_filter) {
			ret = m_filter->filter_add_frame(m_pFrame);
			if (ret < 0 && AVERROR(EAGAIN) != ret) {
				AS_LOG(AS_LOG_WARNING, "audio filter add fail, break...");
				break;
			}

			while (true) {
				ret = m_filter->filter_get_frame(m_pFrame);
				if (ret < 0)
					break;

				if (!(af = m_videoState->sampq.peekWritable()))
					break;

				af->pts = m_pFrame->pts;
				af->serial = m_serial;
				av_frame_move_ref(af->frame, m_pFrame);
				m_videoState->sampq.queuePush();
			}
		}
		else {
			if (!(af = m_videoState->sampq.peekWritable()))
				break;

			af->pts = m_pFrame->pts;
			af->serial = m_serial;
			av_frame_move_ref(af->frame, m_pFrame);
			m_videoState->sampq.queuePush();
		}*/

		AVRational tb = { 1, 1000 };
		if (m_videoState->audio_st) {
			tb = m_videoState->audio_st->time_base;
		}
		pts = (m_frame->pts == AV_NOPTS_VALUE) ? NAN : m_frame->pts * av_q2d(tb);
		duration = m_frame->nb_samples / (double)m_frame->sample_rate;
		//AS_LOG(AS_LOG_WARNING, "decoded audio frame[%lld], pts[%f] duration[%f]", m_pFrame->pts, pts, duration);
		m_videoState->audclk.setClock(pts, m_serial);
		if (kNormal == m_videoState->play_mode) {  //rtsp不参考音频,不用音频来重置外部时钟
			m_videoState->extclk.sync2Slave(&m_videoState->audclk);
		}
		processFrame(m_frame);
		av_frame_unref(m_frame);
		AVSleep(duration);
	}
	return ret;
}

int AvsAudioThread::processFrame(AVFrame* frame)
{
	if (!m_needDisplay || !frame || NULL == frame->data[0]) {
		return AS_ERROR_CODE_AGAIN;
	}

	AVFrame* dstFrame = frame;
	int ret = 0;
	if (m_filter) {
		ret = m_filter->filter_add_frame(dstFrame);
		if (ret < 0 && AVERROR(EAGAIN) != ret) {
			return ret;
		}

		while (true) {
			ret = m_filter->filter_get_frame(dstFrame);
		    if (ret < 0) {
				break;
			}
			
			if (m_audioPlayer) {
				m_audioPlayer->push(dstFrame);
			}
			else {
				AvsAudioPlay::GetInstance()->push(dstFrame);
			}
		}
	}
	else {
		if (m_audioPlayer) { 
			m_audioPlayer->push(dstFrame);
		}
		else {
			AvsAudioPlay::GetInstance()->push(dstFrame);
		}
	}
	return 0;
}



int AvsAudioThread::resample(AVFrame* frame, int & resampled_data_size, uint8_t*** converted_input_samples)
{
	AVFrame* dstFrame = frame;
	int ret = 0;
	//uint8_t** converted_input_samples = &m_videoState->audio_buf1;

	ret = m_resample->resample(dstFrame, converted_input_samples, &resampled_data_size);
	if (ret < 0) {
		AS_LOG(AS_LOG_ERROR, "Could not resample to play.");
	}
	return ret;
}

int AvsAudioThread::initResample(int64_t layout, AVSampleFormat format, int sampleRate, int nbSamples)
{
	if (!m_resample && !(m_resample = AS_NEW(m_resample))) {
		return AVERROR(ENOMEM);
	}
	m_resample->setParameters(layout, format, sampleRate, nbSamples);
	return 0;
}

void AvsAudioThread::setVolume(int16_t volume)
{
	if (volume <= 0) {
		m_needDisplay = false;
		m_pktq->packetqFlush();
	}
	else {
		if (!getPaused()) {
			m_needDisplay = true;
		}
	}
	
	if (m_volume != volume) {
		m_volume = volume;
	#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
		if (volume >= 0) {
			if (m_audioPlayer) {
				m_audioPlayer->setVolume(volume);
			}
			else {
				AvsAudioPlay::GetInstance()->initAudioPara(m_codecParameters);
				AvsAudioPlay::GetInstance()->setVolume(volume);
			}
		}
	#endif
	}
}

//void AvsAudioThread::setScale(double scale) {
//	if (nullptr == m_filter) {
//		m_filter = new AvsAudioFilter();
//		if (!m_filter || !m_filter->init(m_videoState->id)) 
//			return;
//	}
//	char afilter[32] = {0};
//#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
//	sprintf_s(afilter, "atempo=%lf", scale);
//#else
//	sprintf(afilter, "atempo=%lf", scale);
//#endif
//	std::string filter_str(afilter);
//	m_filter->parse_filter_params(filter_str);
//}

void AvsAudioThread::pause(bool isPause, bool needClear)
{
	m_needDisplay = (!isPause && (m_volume > 0));
	AvsDecodeThread::pause(isPause, needClear);
#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
	AvsAudioPlay::GetInstance()->pause(isPause);
#endif
}
