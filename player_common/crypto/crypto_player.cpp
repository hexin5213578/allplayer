#include "crypto_player.h"
#include "avs_video_thread.h"
#include "avs_audio_thread.h"
#include "avs_audio_play.h"
#include "avs_decoder.h"

extern int kULimit;
using namespace crypto;

CryptoPlayer::CryptoPlayer(BizParams& params) : AvsPlayer(params),
	m_cryptoFile(params.Url),
	m_mediaBuf(nullptr),
	m_reader(nullptr), m_parser(nullptr) {
	this->audio_volume = params.VolumeControl;
	m_duration = 0;
	play_mode = kNormal;
	cached_enough = 1;
}

CryptoPlayer::~CryptoPlayer() {
	close();
}

int CryptoPlayer::play() {
	if (!(m_reader = new CryptoReader())) {
		return AVERROR(ENOMEM);
	}

	if (AS_ERROR_CODE_OK != m_reader->init(m_cryptoFile, this)) {
		return AVERROR(EFAULT);
	}
	
	int ret = m_reader->start_ingress();
	if (ret < 0 || !m_reader->get_format_context()) {
		return ret;
	}

	m_view = std::shared_ptr<AvsVideoView>(ViewFactory::getInstance()->createVideoView(m_hwnd), [](AvsVideoView* view) {
			ViewFactory::getInstance()->detroyViewView(view->getHwnd());
		});

	if (!m_view) {
		return AVERROR(EINVAL);
	}
	startRun();
	return 0;
}

int CryptoPlayer::initOutput() {
	auto format = m_reader->get_format_context();
	if (!format) {
		return AVERROR(EINVAL);
	}

	int ret = 0;
	do {
		if (!m_parser && !(m_parser = new AvsStreamParser())) {
			ret = AVERROR(ENOMEM);
			break;
		}
		else {
			m_parser->close();
		}
		
		if ((ret = m_parser->init(format)) < 0) {
			break;
		}
		m_parser->addTask(this);
		
		if (((void*)INVALID_WND != m_hwnd) && (format->video_stream >= 0) &&
			!m_videoThread && !(m_videoThread = new AvsVideoThread(this))) {
			ret = AVERROR(ENOMEM);
			break;
		}

		auto videos = format->streams[format->video_stream];
		if (m_videoThread->openSoftDecoder(videos)) {
			break;
		}

		if (!m_videoThread->getStarted()) {
			m_videoThread->startRun();
		}

		if (format->audio_stream >= 0) {
			if (!m_audioThread && !(m_audioThread = new AvsAudioThread(this))) {
				ret = AVERROR(ENOMEM);
				break;
			}
			auto audios = format->streams[format->audio_stream];
			AVCodecParameters ffpar;
			ffpar.extradata = NULL;
			CodecParasCopy(&ffpar, audios->codecpar);
			m_audioThread->initAudioPara(&ffpar, audio_volume);
			m_audioThread->setVolume(audio_volume);
			if (m_audioThread->openSoftDecoder(audios)) {
				break;
			}

			if (!m_audioThread->getStarted()) {
				m_audioThread->startRun();
			}
		}

		if (m_videoThread) {
			if (!(m_refreshLoop = new AvsRefreshLoop(this))) {
				return AVERROR(ENOMEM);
			}

			if (m_refreshLoop->setView(m_view) < 0) {
				return AVERROR(EINVAL);
			}

			m_duration = format->duration;
			m_refreshLoop->setDuration(m_duration/ 1000);
			//m_refreshLoop->setDuration(m_duration);
			m_refreshLoop->startRun();
		}
	} while (0);

	if (ret < 0) {
		if (m_parser) {
			m_parser->close();
			delete m_parser;
			m_parser = nullptr;
		}

		if (m_videoThread) {
			m_videoThread->stopRun();
			delete m_videoThread;
			m_videoThread = nullptr;
		}

		if (m_audioThread) {
			m_audioThread->stopRun();
			delete m_audioThread;
			m_audioThread = nullptr;
		}
	}
	return ret;
}

int32_t CryptoPlayer::handle_ingress_status(MEDIA_STATUS_INFO status) {
	int ret = 0;
	AVS_EGRESS_STATUS st = AVS_STATUS_NONE;
	switch (status.enStatus) {
	case kCheckHeaderFaild:
		st = AVS_INVALID;
		break;
	case kParsedMediaAttribute:
		ret = initOutput();
		if (ret < 0) {
			st = AVS_INVALID;
		}
		break;
	case kAllocResourceFaild:
		st = AVS_PLAY_FAILED;
		break;
	default:
		break;
	}

	if (st != STREAM_STATUS_NONE) {
		sendStatus(st, "");
	}
	return ret;
}

char* CryptoPlayer::alloc_ingress_data_buf(uint32_t len, uint32_t& ulBufLen) {
	if (RECV_DATA_BUF_SIZE < len) {
		return nullptr;
	}
	
	ulBufLen = RECV_DATA_BUF_SIZE;
	if (!m_mediaBuf && !(m_mediaBuf = new char[RECV_DATA_BUF_SIZE])) {
		return nullptr;
	}
	return &m_mediaBuf[sizeof(MediaInfoMsg)];
}

int32_t CryptoPlayer::handle_ingress_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len) 
{
	if (!m_reader || !m_parser) {
		return AS_ERROR_CODE_FAIL;
	}
	
	auto format = m_reader->get_format_context();
	if (!format) {
		return AS_ERROR_CODE_FAIL;
	}
	
	int32_t headerSize = sizeof(MediaInfoMsg);
	auto media_msg = (MediaInfoMsg*)m_mediaBuf;
	media_msg->dataInfo = *dataInfo;
	media_msg->dataSize = len;
	m_parser->parseMediaData(media_msg);
	return 0;
}

int CryptoPlayer::readUntil(int64_t target) {
	int64_t pts = 0;
	int ret = 0, got = 0, type = -1;

	for (;;) {
		if ((ret = m_reader->readFrameData(pts, type)) < 0) {
			break;
		}
		
		if (pts >= target - 1) {
			got = 1;
			break;
		}
	};
	ret = got;
	return ret;
}


void CryptoPlayer::mainProcess()
{
	if (!m_reader->get_format_context()) {
		return;
	}
	
	video_stream = m_reader->get_format_context()->video_stream;
	audio_stream = m_reader->get_format_context()->audio_stream;

	std::mutex wait_mutex;
	int ret = 0;
	static const double kWaitResume = 0.05, kWaitGet = 0.015;
	static const double kDuration = 0.04;
	int64_t pts = 0; int type = 0;
	
	while (!m_bIsExit) {
		if (this->abort_request) {
			break;
		}

		if (this->paused != this->last_paused) {
			this->last_paused = this->paused;
			if (this->paused) {
				m_reader->pause_ingress();
				this->read_pause_return = 1;
			}
			else {
				m_reader->pause_ingress();
			}
		}

		if (this->seek_req) {
			int64_t seek_target = this->seek_pos;
			if((ret = m_reader->vcr_control(this->m_seekFrac, 1.0)) < 0) {
				AS_LOG(AS_LOG_WARNING, "%s: error while seeking, ignore request.", this->m_cryptoFile.data());
			}
			else {
				if (this->audio_stream >= 0) {
					this->audioq->packetqFlush();
				}
				if (this->video_stream >= 0) {
					this->videoq->packetqFlush();
				}

				/*if (m_videoThread) m_videoThread->threshold(seek_target);
				int got = readUntil(seek_target);
				if (got <= 0) {
					AS_LOG(AS_LOG_WARNING, "%s: seek doesn't precisely.", this->ic->url);
				}*/
				this->extclk.setClock(seek_target / 1000.0, 0);
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

		static const int kMaxQueueSize = 64;

		if ((vpktq + apktq > kMaxQueueSize || (v_enough_pkts && a_enough_pkts))) {
			std::unique_lock<decltype(wait_mutex)> lck(wait_mutex);
			continue_read_cond.wait_for(lck, std::chrono::milliseconds(10));
			continue;
		}

		ret = m_reader->readFrameData(pts, type);
		if (ret < 0) {
			if (AVERROR_EOF == ret && !this->eof) {
				if (this->video_stream >= 0) {
					this->videoq->putNullPacket(this->video_stream);
				}
				if (this->audio_stream >= 0) {
					this->audioq->putNullPacket(this->audio_stream);
				}
				this->eof = 1;
			}

			if (kFileIOError == ret || kInvalidData == ret) {
				sendStatus(AVS_PLAY_FAILED, "");
				break;
			}
			std::unique_lock<decltype(wait_mutex)> lck(wait_mutex);
			continue_read_cond.wait_for(lck, std::chrono::milliseconds(10));
			continue;
		}
		else {
			this->eof = 0;
		}
	}
}

int CryptoPlayer::doTask(AVPacket* pkt)
{
	int ret = 0;
	if (pkt->stream_index == video_stream) {
		if (m_videoThread) {
			m_videoThread->pushPacket(pkt);
		}
	}
	else if (pkt->stream_index == audio_stream) {
		if (m_audioThread) {
			m_audioThread->pushPacket(pkt);
		}
	}
	else {
		ret = AVERROR(EINVAL);
	}
	return ret;
}

void CryptoPlayer::close() {
	AvsThreadBase::stopRun();
	wait();

	if (m_refreshLoop) {
		m_refreshLoop->stopRun();
		AS_DELETE(m_refreshLoop);
	}

	if (m_reader) {
		delete m_reader;
		m_reader = nullptr;
	}

	pictq->signal();
	if (m_videoThread) {
		m_videoThread->stopRun();
		delete m_videoThread;
		m_videoThread = nullptr;
	}

	sampq->signal();
	if (m_audioThread) {
		m_audioThread->stopRun();
		AS_DELETE(m_audioThread);
	}

	if (m_parser) {
		delete m_parser;
		m_parser = nullptr;
	}

	if(m_mediaBuf) {
		delete m_mediaBuf;
		m_mediaBuf = nullptr;
	}
}

void CryptoPlayer::pause(bool pause, bool clear)
{
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

void CryptoPlayer::seek(double frac)
{
	if (!this->seek_req && m_duration > 0) {
		int64_t ts = frac * m_duration;
		this->seek_pos = ts;
		this->m_seekFrac = frac;
		this->seek_req = 1;
	}
}

std::string CryptoPlayer::getPlayInfo()
{
	if (m_reader) {
		auto header = m_reader->getHeader();
		auto root = cJSON_CreateObject();

		if (header.limit_times < kULimit) {
			auto remaining_times = cJSON_CreateNumber(header.limit_times - header.use_times);
			cJSON_AddItemToObject(root, "remaining_times", remaining_times);
		}

		if (header.limit_day < kULimit) {
			time_t ts = header.create_time + header.limit_day * 24 * 60 * 60;
			auto deadline = cJSON_CreateString(getTimeStr("%Y-%m-%d", ts).data());
			cJSON_AddItemToObject(root, "deadline", deadline);
		}

		auto duration = cJSON_CreateNumber(header.total_duration / 1000);
		cJSON_AddItemToObject(root, "duration", duration);

		auto info_sz = cJSON_Print(root);
		auto play_info = std::string(info_sz);
		free(info_sz);
		cJSON_Delete(root);
		return play_info;
	}
	return std::string();
}


