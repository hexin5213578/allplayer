#include "avs_decdoe_thread.h"
#include "avs_decoder.h"
#include "avs_player_factory.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#if ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include <unistd.h>
#include "openGL_video_view.h"
#include "../../../Prj-Android/allplayer/src/main/cpp/AndroidLog.h"
#endif

#include "avs_player_factory.h"

string ffmpeg_err(int errnum) {
	char errbuf[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
	return errbuf;
}

AvsDecodeThread::AvsDecodeThread(VideoState* ic): m_videoState(ic) {
	if (!m_videoState) {
		throw std::invalid_argument("invalid ic setted for decoder.");
	}
	m_frame = av_frame_alloc();
}

AvsDecodeThread::~AvsDecodeThread() {
	av_frame_free(&m_frame);
}

int AvsDecodeThread::openSoftDecoder(MK_Stream* stream) {
	if (!stream || !stream->codecpar) {
		return AVERROR(EINVAL);
	}

	std::unique_lock<std::mutex> lock(m_ctxMutex);
	if (!m_decoder && !(m_decoder = AS_NEW(m_decoder))) {
		return AVERROR(ENOMEM);
	}

	int ret = 0;
	m_decoder->close();
	if (ret = m_decoder->initMK(stream)) {
		AS_DELETE(m_decoder);
		return ret;
	}
	return ret;
}

int AvsDecodeThread::openSoftDecoder(AVStream* stream) {
	std::unique_lock<std::mutex> lock(m_ctxMutex);
	if (!m_decoder && !(m_decoder = AS_NEW(m_decoder))) {
		return AVERROR(ENOMEM);
	}
	
	int ret = 0;
	m_decoder->close();
	if (ret = m_decoder->initFF(stream)) {
		AS_DELETE(m_decoder);
		return ret;
	}
	m_duration = stream->duration;
	return ret;
}

void AvsDecodeThread::closeDecoder() {
	std::unique_lock<std::mutex> lock(m_ctxMutex);
	AS_DELETE(m_decoder);
}

void AvsDecodeThread::mainProcess() {
	//m_pktq->start();
	this->decoding();
}

int AvsDecodeThread::decodeFrame() {
	int ret = AVERROR(EAGAIN);

	for (;;) {
		AVPacket pkt;

		do {
			unique_lock<mutex> lock(m_ctxMutex);
			if (!m_decoder) {
				return 0;
			}
		} while (0);

		if (m_pktq->serial() == m_serial) {
			do {
				if (m_bIsExit) {
					return -1;
				}
				
				ret = m_decoder->recvFrameInt(m_frame);
				if (AVERROR_EOF == ret || (ret < 0 && m_arriveEOF)) {
					if (AVERROR_EOF != ret && AVERROR(EAGAIN) != ret) {
						AS_LOG(AS_LOG_ERROR, "receive frame error : %s", ffmpeg_err(ret).data());
					}
					if (m_finished != m_serial && m_pktq == m_videoState->videoq) {
						m_videoState->sendStatus(AVS_EOF, "eof");
						AS_LOG(AS_LOG_NOTICE, "biz[%ld] arrive eof.", m_videoState->id);
					}
					m_finished = m_serial;
					m_decoder->flushCodecBuffer();
					return 0;
				}
				if (ret >= 0)  {
					return 1;
				}
			} while (ret != AVERROR(EAGAIN));
		}

		do {
			if (m_pktPending) {
				av_packet_move_ref(&pkt, &m_pendPkt);
				m_pktPending = 0;
			}
			else {
				int old_serial = m_serial;
				ret = m_pktq->packetGet(&pkt, 1, &m_serial);
				if (ret <= 0) {
					return ret;
				}
	
				//if (m_stepDirect) {		//step cause pause.wait for pause/resume to stop step
				//	if (!m_paused) {
				//		if (m_stepDirect < 0) {
				//			auto size = m_pktq->packetSize();
				//			AS_LOG(AS_LOG_NOTICE, "biz[%ld] left %d packets when backward", m_videoState->id, size);
				//		}
				//		m_paused = true;
				//		NotiData data;
				//		data.type = NotiData::STEPEND;
				//		notify(data);
				//	}
				//}
					
				if (old_serial != m_serial) {
					m_finished = 0;
					m_decoder->flushCodecBuffer();
				}
			}

			if (0 == pkt.size && 0 == pkt.data)	{	//0 == pkt.size,最后一帧pkt 
				m_arriveEOF = true;
				break;
			}

			if(m_pktq->serial() == m_serial) {
				bool skippable = m_skipNonKey && !(pkt.flags & AV_PKT_FLAG_KEY);
				if (!skippable) {
					break;
				}
			}
			av_packet_unref(&pkt);
		} while (1);

		if(1) {
#ifdef _DEBUG
			auto t1 = getRelativeTime();
#endif //
			if (m_decoder->sendPkt(&pkt) == AVERROR(EAGAIN)) {
				AS_LOG(AS_LOG_WARNING, "biz[%lld] receive_frame and send_packet both returned EAGAIN, which is an API violation.");
				m_pktPending = 1;
				av_packet_move_ref(&m_pendPkt, &pkt);
			}
#ifdef _DEBUG
			static const double kHold = 0.04;
			auto t2 = getRelativeTime();
			if (t2 - t1 > kHold) {
				AS_LOG(AS_LOG_INFO, "send packet use [%llf]s, rest %d packet.", t2 - t1, m_pktq->packetSize());
			}
#endif  //
		}
		av_packet_unref(&pkt);		
	}
}

void AvsDecodeThread::clear() {
	m_pktq->packetqFlush();
	if (m_decoder) {
		m_decoder->clear();
	}
}

void AvsDecodeThread::seek() {
	clear();
}

void AvsDecodeThread::pause(bool pause, bool clear) {
	AvsThreadBase::setPause(pause);
	if (pause && (clear || m_videoState->realtime)) {
		m_pktq->packetqFlush();
		unique_lock<mutex> lock(m_ctxMutex);
		if (m_decoder) {
			m_decoder->clear();
		}
	}
}

void AvsDecodeThread::stopRun() {
	m_pktq->abort();
	AvsThreadBase::stopRun();
	//释放decode缓存，防止内存泄漏
	wait();
	m_pktq->packetqFlush();
	closeDecoder();
}

void AvsDecodeThread::restart() {
	m_pktq->packetqFlush();
}

void AvsDecodeThread::skipNonKey(bool skip) {
	m_skipNonKey = skip;
	if (m_pktq) {
		m_pktq->packetqFlush();
		std::unique_lock<std::mutex> lock(m_ctxMutex);
		if (m_decoder) {
			m_decoder->flushCodecBuffer();
		}
	}
}
