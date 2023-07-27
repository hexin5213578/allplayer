#include "avs_universal_player.h"
#include "as_config.h"
#include "avs_demuxer.h"
#include "avs_video_thread.h"
#include "avs_audio_thread.h"
#include "avs_player_common.h"
#include "avs_decoder.h"
#include "avs_ios_refresh_loop.h"

#if((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include "ios_video_thread.h"
#include "ios_audio_thread.h"
#endif

AvsUniversalPlayer::AvsUniversalPlayer(BizParams& params) 
	: AvsPlayer(params),
	m_strUrl(params.Url) {
	this->play_mode = kNormal;
	cached_enough = 1;
}

int AvsUniversalPlayer::openInput() {
	if (end_with(m_strUrl, ".avs")) {
		AS_LOG(AS_LOG_ERROR, "invalid format avs.");
		return AVERROR(EINVAL);
	}

	closeInput();
	if (!(m_demux = new AvsDemuxer(this))) {
		return AVERROR(ENOMEM);
	}

	int ret = m_demux->openInput(m_strUrl);
	if (ret < 0) {
		AS_DELETE(m_demux);
		return ret;
	}

	if (m_infinitBuffer < 0 && realtime) {
		m_infinitBuffer = 1;
	}

	if (video_stream >= 0) {
        m_videoThread = new AvsVideoThread(this);

		if (!m_videoThread) {
			return AVERROR(ENOMEM);
		}

		if ((ret = m_videoThread->setDecodePara(video_st, m_hardwareAcc)) < 0) {
			return ret;
		}
	}

	if (audio_stream >= 0) {
        if (!m_audioThread) {
#if ((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
            m_audioThread = new IOSAudioThread(this);
#else
            m_audioThread = new AvsAudioThread(this);
#endif
            if (!m_audioThread) {
                return AVERROR(ENOMEM);
            }
        }

#if ((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
        m_audioThread->initAudioPara(audio_st->codecpar, this->audio_volume);
#endif
		
		ret = m_audioThread->openSoftDecoder(audio_st);
		if (ret < 0) {
			AS_LOG(AS_LOG_WARNING, "open audio decoder faild.");
			ret = 0;
		}
	}

	int tns = ic->duration / 1000000LL;
	sendData(0, tns);
    return ret;
}

int AvsUniversalPlayer::play() {
	int ret = openInput();
	if (ret < 0) {
		return ret;
	}

	if (!m_demux || !m_videoThread) {
		return AVERROR(EINVAL);
	}


#if((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
	m_refreshLoop = new AvsIosRefreshLoop(this);
#else
	m_refreshLoop = new AvsRefreshLoop(this);
#endif

	if (!m_refreshLoop) {
		return AVERROR(ENOMEM);
	}
	
	if ((void*)INVALID_WND != m_hwnd) {
		auto view = std::shared_ptr<AvsVideoView>(ViewFactory::getInstance()->createVideoView(m_hwnd), [](AvsVideoView* view) {
            if(view) {
                ViewFactory::getInstance()->detroyViewView(view->getHwnd());
            }
        });

#if((AS_APP_OS & AS_OS_IOS) != AS_OS_IOS)
        if (!view) {
            return AVERROR(EINVAL);
        }
#endif
		if (m_refreshLoop->setView(view) < 0) {
			return AVERROR(EINVAL);
		}
        m_refreshLoop->setWindow(m_hwnd);
	}
	
	m_refreshLoop->startRun();
	m_videoThread->startRun();
	if (m_audioThread) {
		m_audioThread->startRun();
	}
	this->startRun();
	return 0;
}

void AvsUniversalPlayer::mainProcess() {
	this->playRoute();
}

void AvsUniversalPlayer::playRoute()
{
	AVPacket* pkt = av_packet_alloc();
	std::mutex wait_mutex;

    int ret = 0;
	static const double kWaitResume = 0.05, kWaitGet = 0.015;
	static const double kDuration = 0.04;

	while (!m_bIsExit) {
		if (this->abort_request) {
			break;
		}

		if (this->paused != this->last_paused) {
			this->last_paused = this->paused;
			if (this->paused) {
				this->read_pause_return = av_read_pause(this->ic);
			}
			else {
				av_read_play(this->ic);
			}
		}

		if (this->seek_req) {
			int64_t seek_target = this->seek_pos;
			int64_t seek_min = this->seek_rel > 0 ? seek_target - this->seek_rel + 2 : INT64_MIN;
			int64_t seek_max = this->seek_rel < 0 ? seek_target - this->seek_rel - 2 : INT64_MAX;

			ret = avformat_seek_file(this->ic, -1, seek_min, seek_target, seek_max, this->seek_flags);
			if (ret < 0) {
				AS_LOG(AS_LOG_WARNING, "%s: error while seeking.", this->ic->url);
			}
			else {
				if (this->audio_stream >= 0)
					this->audioq->packetqFlush();
				if (this->video_stream >= 0)
					this->videoq->packetqFlush();

				int got = readUntil(seek_target, pkt);
				if (got <= 0) {
					AS_LOG(AS_LOG_WARNING, "%s: seek doesn't precisely.", this->ic->url);
				}

				if (this->seek_flags & AVSEEK_FLAG_BYTE) {
					this->extclk.setClock(NAN, 0);
				}
				else {
					this->extclk.setClock(seek_target / (double)AV_TIME_BASE, 0);
				}
			}

			this->seek_req = 0;
			this->eof = 0;
			if (this->paused) {
				pause(false);
			}
		}

		int vpktq = m_videoThread ? m_videoThread->getPktqSize() : 0;
		int apktq = m_audioThread ? m_audioThread->getPktqSize() : 0;

		int v_enough_pkts = m_videoThread ? m_videoThread->enoughPkts() : 1;
		int a_enough_pkts = m_audioThread ? m_audioThread->enoughPkts() : 1;

		static const int kMaxQueueSize = 32 * 1024;

		if ((m_infinitBuffer < 1) &&
			(vpktq + apktq > kMaxQueueSize || (v_enough_pkts && a_enough_pkts)))
		{
			std::unique_lock<decltype(wait_mutex)> lck(wait_mutex);
			continue_read_cond.wait_for(lck, std::chrono::milliseconds(10));
			continue;
		}

		ret = m_demux->read(pkt);
		if (ret < 0) {
			if ((AVERROR_EOF == ret || avio_feof(ic->pb)) && !eof) {
				if (this->video_stream >= 0)
					this->videoq->putNullPacket(this->video_stream);

				if (this->audio_stream >= 0)
					this->audioq->putNullPacket(this->audio_stream);
				eof = 1;
			}
			std::unique_lock<decltype(wait_mutex)> lck(wait_mutex);
			continue_read_cond.wait_for(lck, std::chrono::milliseconds(10));
			continue;
		}
		else {
			eof = 0;
		}

		if (pkt->stream_index == video_stream) {
		}
		else if (pkt->stream_index == audio_stream) {
		}
		else {
			av_packet_unref(pkt);
			continue;
		}

		if ((pkt->stream_index == video_stream) && m_videoThread) {
			m_videoThread->pushPacket(pkt);
		}
		else if ((pkt->stream_index == audio_stream) && m_audioThread) {
			m_audioThread->pushPacket(pkt);
		}
		else {
			av_packet_unref(pkt);
		}
	}
	av_packet_free(&pkt);
}

int AvsUniversalPlayer::readUntil(int64_t target, AVPacket* pkt)
{
	int ret = 0;
	if (!viddec) {
		return AVERROR(EINVAL);
	}
	AVFrame* vf = av_frame_alloc();
	if (!vf) {
		return AVERROR(ENOMEM);
	}
	int got = 0;
	for (;;) {
		ret = m_demux->read(pkt);
		if (ret < 0) {
			break;
		}

		if (pkt->stream_index == video_stream) {
			ret = viddec->sendPkt(pkt);
			if (ret < 0 && AVERROR_EOF != ret) {
				break;
			}

			while (ret >= 0 && !got) {
				ret = viddec->recvFrameInt(vf);
				if (ret >= 0) {
					double ts = vf->pts * av_q2d(video_st->time_base);
					if (ts >= target / (double)AV_TIME_BASE - 0.001) {
						got = 1;
					}
				}
				av_frame_unref(vf);
			}
		}

		av_packet_unref(pkt);
		if (got) {
			break;
		}
	}
	av_frame_free(&vf);
	ret = got;
	return ret;
}

void AvsUniversalPlayer::closeInput()
{
	if (m_demux) {
		m_demux->close();
		AS_DELETE(m_demux);
	}
}

void AvsUniversalPlayer::seek(double frac)
{
	if (!this->ic) return;

	if (!this->seek_req) {
		int64_t ts = frac * ic->duration;
		this->seek_pos = ts;
		this->seek_rel = 0;
		this->seek_flags &= ~AVSEEK_FLAG_BYTE;
		this->seek_req = 1;
		this->continue_read_cond.notify_all();
	}
}

void AvsUniversalPlayer::pause(bool pause, bool clear) {
	if (this->paused) {
		this->frame_timer += getRelativeTime() - this->vidclk.last_updated;
		if (this->read_pause_return != AVERROR(ENOSYS)) {
			this->vidclk.paused = 0;
		}
		this->vidclk.setClock(this->vidclk.getClock(), this->vidclk.serial);
	}
	this->extclk.setClock(this->extclk.getClock(), this->extclk.serial);
	this->paused = this->audclk.paused = this->vidclk.paused = this->extclk.paused = !this->paused;
}

void AvsUniversalPlayer::stopRun() {
	AvsThreadBase::stopRun();
	wait();
	AvsPlayer::stop();
	closeInput();
}




