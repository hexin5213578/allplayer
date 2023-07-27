#include "avs_rtsp_processor.h"
#include "avs_player_common.h"
#if((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include "track.h"
#include "as_frame.h"
#elif ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "track/track.h"
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "track/track.h"
#include "extend/as_frame.h"
#endif

//#define VIDEO_DUMP_FILE
//#define AUDIO_DUMP_FILE

static const char* RTSP_PREFIX = "rtsp://";
static std::string splitIpFromUrl(std::string url) {
	std::string ip;
	size_t prefixLength = strlen(RTSP_PREFIX);
	if (url.length() > prefixLength) {
		int pos = url.find_first_of("/", prefixLength);
		if (std::string::npos != pos) {
			ip = url.substr(prefixLength, pos - prefixLength);
			//ȥ��rtsp url ipv6��ʽ��'['��']'
			if (!ip.empty()) {
				ip.erase(std::remove_if(ip.begin(), ip.end(), [](char c) {
					return (c == '[' || c == ']');
					}));
			}
		}
	}
	return ip;
}

static const int kRtspRecvSize = 2 * 1024 * 1024;
static const int kMultiRtspRecvSize = 9 * 1024 * 1024;

#define ADD_NUMBER_PARAM_TO_JSON(p,i,root)  do {											\
												cJSON* pJson = cJSON_CreateNumber(i);						\
												if(pJson) 	cJSON_AddItemToObject(root,p,pJson);			\
											}while(0)


#define ADD_STRING_PARAM_TO_JSON(p,s,root)  do {											\
												cJSON* pJson = cJSON_CreateString(s);						\
												if(pJson) 	cJSON_AddItemToObject(root,p,pJson);			\
											}while(0)


AvsRtspProcessor::AvsRtspProcessor(){
	m_statisMediaSize = { 0,0,0,0 };
	m_mediaInfo.video_fomrat = UINT8_MAX;
	m_mediaInfo.audio_format = UINT8_MAX;
	memset(&m_statistic, 0, sizeof m_statistic);
}

AvsRtspProcessor::~AvsRtspProcessor() {
	AS_DELETE(m_ingress);
	AS_DELETE(m_mediaBuf, MULTI);
	AS_DELETE(m_parser);
}

int AvsRtspProcessor::init_1(AvsPlayer* master) {
	assert(master);
	m_master = master;
	int ret = 0;
	do {
		if (TYPE_MULTI_REALVIDEO_START == m_master->type || TYPE_MULTI_RECORD_START == m_master->type) {
			m_mediaBuf = AS_NEW(m_mediaBuf, kMultiRtspRecvSize);
			m_buffSize = kMultiRtspRecvSize;
		}
		else {
			m_mediaBuf = AS_NEW(m_mediaBuf, kRtspRecvSize);
			m_buffSize = kRtspRecvSize;
		}
		if (!m_mediaBuf) {
			AS_LOG(AS_LOG_WARNING, "memory out");
			ret = -1;
			break;
		}
        if (!m_ingress) {
            m_ingress = AS_NEW(m_ingress);
        }
		if (!m_ingress) {
			AS_LOG(AS_LOG_WARNING, "memory out");
			ret = -1;
			break;
		}
	} while (0);

	if (ret < 0) {
		AS_DELETE(m_mediaBuf, MULTI);
		AS_DELETE(m_ingress);
	}
	return 0;
}

int AvsRtspProcessor::start_1() {
	if (!m_mediaBuf || !m_ingress || kStopped != m_status) {
		return -1;
	}

	m_loadStartMs = getNowMs();
	auto p1 = m_master->getBizParams();
	m_mediaInfo.serverIP = splitIpFromUrl(p1.Url);

	int ret = 0;
	do {
		m_ingress->set_vcr_initial_parameter(p1.vcrSt, p1.HighSpeedCache, p1.BizType);
		if(ret = m_ingress->init(p1.Url, this)) {
			break;
		}

		int streams = p1.StitchInfo.streams;
		if (TYPE_MULTI_RECORD_START == p1.BizType) {
			streams = -streams;
		}
		ret = m_ingress->start_ingress(streams, false);
		if (ret) {
			break;
		}
		m_curBitRateStartMs = getNowMs();
		m_meanBitRateStartMs = m_curBitRateStartMs;
		m_status = kConnecting;
	} while (0);

	if (!ret) {
		AS_LOG(AS_LOG_INFO, "player[%ld] start play, url [%s]!", m_master->id, m_master->getBizParams().Url.c_str());
	}
	return ret;
}

void AvsRtspProcessor::stop_1() {
	if (kStopped == m_status) {
		AS_LOG(AS_LOG_WARNING, "player [%ld] has stopped, return.", m_master->id);
		return;
	}

	RTP_PACKET_STAT_INFO stat;
	memset(&stat, 0, sizeof(stat));
	double pktLostRate = 0.0;
	uint64_t allStreamSizeKb = 0;
	if (m_ingress && (0 == m_ingress->get_stream_stat(stat))) {
		if ((stat.ulLostRtpPacketNum + stat.ulTotalPackNum) > 0) {
			pktLostRate = (double)stat.ulLostRtpPacketNum / ((double)stat.ulTotalPackNum + (double)stat.ulLostRtpPacketNum);
		}
		allStreamSizeKb = stat.ulStreamSizeKb;
	}

	cJSON* root = cJSON_CreateObject();
	char* mediaInfoStr = nullptr;
	if (root) {
		ADD_NUMBER_PARAM_TO_JSON("streamSizeKb", allStreamSizeKb, root);
		mediaInfoStr = cJSON_Print(root);
		cJSON_Delete(root);
	}

	if (m_ingress) {
		m_ingress->stop_ingress();
	}
	AS_DELETE(m_parser);

	m_master->sendStatus(STREAM_STATUS_STOP, mediaInfoStr ? std::string(mediaInfoStr) : "");
	AS_DELETE(mediaInfoStr, 1);
	m_status = kStopped;
	AS_LOG(AS_LOG_INFO, "player[%ld] stop normally!", m_master->id);

#ifdef  VIDEO_DUMP_FILE
	for (int i = 0; i < STREAM_NB; ++i) {
		m_videoFile[i].close();
	}
#endif //  VIDEO_DUMP_FILE

#ifdef  AUDIO_DUMP_FILE
	if (m_auidoFile.is_open()) {
		m_auidoFile.close();
	}
#endif //  AUDIO_DUMP_FILE
	return;
}

int AvsRtspProcessor::reload_1(BizParams& params) {
	if (!m_ingress || !m_master) {
		return AS_ERROR_CODE_NOT_INIT;
	}

	m_mediaInfo.serverIP = splitIpFromUrl(params.Url);
	m_ingress->stop_ingress();
	m_formatCtx = nullptr;
	m_status = kStopped;

	int ret = 0;
	if ((ret = m_ingress->init(params.Url, this))) {
		AS_LOG(AS_LOG_ERROR, "player[%ld] stream ingress reinit failed, %d !", m_master->id, ret);
		return ret;
	}

	ret = m_ingress->start_ingress(1, false);
	if (ret) {
		AS_LOG(AS_LOG_ERROR, "player[%ld] start stream ingress failed.", m_master->id);
		return ret;
	}
	m_curBitRateStartMs = getNowMs();
	m_meanBitRateStartMs = m_curBitRateStartMs;
	m_status = kConnecting;
	return ret;
}

int AvsRtspProcessor::vcrControl_1(double start, double scale, int scaleOrSpeed) {
	int ret = AS_ERROR_CODE_FAIL;
	if (kConnecting == m_status || kStopped == m_status) {
		return ret;
	}

	static double eps = 0.00001;
	if (kPlaying == m_status && (fabs(start + 1.0) < eps)) {
		if (eps >= abs(m_master->speed - scale)) {
			return 0;
		}
	}

	m_master->sendStatus(STREAM_STATUS_RESUME, "");
	if (m_ingress) {
		//���ٲ���ʱ��̬�ı�scaleOrSpeed
		m_ingress->set_scale_speed(scaleOrSpeed);
	}

	ret = m_ingress->vcr_control(start, scale);
	if (!ret) {
		if (start >= 0) {
			m_seekTimer = start;
			m_seekState = SeekState::kWaitReply;
		}
		m_stepDirect = 0;
		m_stepState = kStepEnd;
	}

	if (start < 0) {
		auto real_speed = VideoState::mapSpeed(scale);
		if (real_speed < 0.0) {
			if (!m_reverse || real_speed < -2.0) {
				m_master->clearBuffer();
			}
			m_reverse = true;
		}
		else if (m_reverse) {
			m_master->clearBuffer();
			m_reversalGop.clear();
			m_reverse = false;
		}
	}
	return ret;
}

void AvsRtspProcessor::pause_1() {
	if (m_ingress && !m_master->realtime) {
		m_ingress->pause_ingress();
	}
	m_stepDirect = kStepEnd;
	m_status = kPaused;
	m_master->sendStatus(STREAM_STATUS_PAUSE, "");
}

void AvsRtspProcessor::resume_1() {
	if (m_ingress && !m_master->realtime)
		m_ingress->vcr_control(-1, m_master->speed);

	m_master->sendStatus(STREAM_STATUS_RESUME, "");
	m_stepDirect = 0;
	m_status = kPlaying;
	m_stepState = kStepEnd;
}

int AvsRtspProcessor::step2NextFrame_1(int8_t direct) {
	if (kStepEnd != m_stepState || !m_ingress || m_master->realtime) {
		return -1;
	}

	bool reverse = false;
	if (m_stepDirect) {				//step+pause state
		reverse = (direct > 0 && m_stepDirect < 0) || (direct < 0 && m_stepDirect > 0);
	}
	else {
		reverse = (direct > 0 && m_master->speed < 0.0) || (direct < 0 && m_master->speed > 0.0);
	}

	if (reverse) {
		m_master->clearBuffer();
	}

	m_stepDirect = direct;
	int stepNb = 0; //= m_master->getStepFrameNB();

	//play when buffer almost empty or reverse
	if (((kPaused == m_status) && (stepNb >= 0 && stepNb <= 2)) || reverse) {
		//m_ingress->vcr_control(-1, m_stepDirect == -1 ? 252: m_stepDirect); 
		m_ingress->vcr_control(-1, m_stepDirect);   // key frame
		if (reverse) {
			m_stepState = kStepWaitForReverse;
		}
	}
	return 0;
}

std::string AvsRtspProcessor::getMediaInfo_1() {
	RTP_PACKET_STAT_INFO stat;
	memset(&stat, 0, sizeof(stat));
	cJSON* root = cJSON_CreateObject();
	if (!root || !m_ingress || m_ingress->get_stream_stat(stat)) {
		AS_LOG(AS_LOG_ERROR, "biz[%ld] get media info fail.", m_master->id);
		return "";
	}

	if (stat.ulTotalPackNum > 0) {
		m_mediaInfo.packet_lost_rate = (double)stat.ulLostRtpPacketNum / ((double)stat.ulTotalPackNum + (double)stat.ulLostRtpPacketNum);
	}
	m_mediaInfo.recv_stream_kb = stat.ulStreamSizeKb;

	if (m_formatCtx && (m_formatCtx->video_stream >= 0)) {
		auto stream = m_formatCtx->streams[m_formatCtx->video_stream];
		if (stream) {
			m_mediaInfo.res_width = stream->codecpar->width;
			m_mediaInfo.res_height = stream->codecpar->height;
		}
	}
	m_mediaInfo.fps = m_master->getFps();
	ADD_NUMBER_PARAM_TO_JSON("width", m_mediaInfo.res_width, root);
	ADD_NUMBER_PARAM_TO_JSON("height", m_mediaInfo.res_height, root);
	ADD_NUMBER_PARAM_TO_JSON("fps", m_mediaInfo.fps, root);
	ADD_NUMBER_PARAM_TO_JSON("videoFormat", m_mediaInfo.video_fomrat, root);
	ADD_NUMBER_PARAM_TO_JSON("videoCurtRate", m_mediaInfo.video_cur_bitrate, root);
	ADD_NUMBER_PARAM_TO_JSON("videoMeanRate", m_mediaInfo.video_mean_bitrate, root);
	ADD_NUMBER_PARAM_TO_JSON("audioCurtRate", m_mediaInfo.audio_cur_bitrate, root);
	ADD_NUMBER_PARAM_TO_JSON("audioMeanRate", m_mediaInfo.audio_mean_bitrate, root);
	ADD_NUMBER_PARAM_TO_JSON("packetLostRate", m_mediaInfo.packet_lost_rate, root);
	ADD_NUMBER_PARAM_TO_JSON("streamSizeKb", m_mediaInfo.recv_stream_kb, root);
	ADD_STRING_PARAM_TO_JSON("serverIP", m_mediaInfo.serverIP.c_str(), root);
	char* mediaInfoStr = cJSON_Print(root);
	std::string mediaInfo(mediaInfoStr);
	cJSON_Delete(root);
	free(mediaInfoStr);
	return mediaInfo;
}

cJSON* AvsRtspProcessor::getExperience_1() {
	RTP_PACKET_STAT_INFO stat;
	cJSON* experience = cJSON_CreateObject();
	if (!experience || !m_ingress || m_ingress->get_stream_stat(stat) || !m_master) {
		AS_LOG(AS_LOG_ERROR, "biz[%ld] get experience data fail.", m_master->id);
		return nullptr;
	}

	double pktLostRate = 0.0;
	uint32_t totalPkts = 0, lostPkts = 0;
	int stalling = 0;
	double stallDuration = 0.0;
	double traffic = stat.dlTraffic - m_statistic.pktStatInfo.dlTraffic;

	if (m_statistic.pktStatInfo.ulTotalPackNum > stat.ulTotalPackNum) {
		totalPkts = (UINT32_MAX - m_statistic.pktStatInfo.ulTotalPackNum) + stat.ulTotalPackNum;
	}
	else {
		totalPkts = stat.ulTotalPackNum - m_statistic.pktStatInfo.ulTotalPackNum;
	}

	if (m_statistic.pktStatInfo.ulLostRtpPacketNum > stat.ulLostRtpPacketNum) {
		lostPkts = (UINT32_MAX - m_statistic.pktStatInfo.ulLostRtpPacketNum) + stat.ulLostRtpPacketNum;
	}
	else {
		lostPkts = stat.ulLostRtpPacketNum - m_statistic.pktStatInfo.ulLostRtpPacketNum;
	}
		
	if (totalPkts + lostPkts > 0) {
		pktLostRate = (double)lostPkts / ((double)totalPkts + (double)lostPkts);
	}

	m_master->getStallingInfo(stalling, stallDuration);
	ADD_NUMBER_PARAM_TO_JSON("bizId", m_master->id, experience);
	ADD_STRING_PARAM_TO_JSON("cameraId", m_master->getBizParams().CameraId.c_str(), experience);
	ADD_NUMBER_PARAM_TO_JSON("traffic", traffic, experience);
	ADD_NUMBER_PARAM_TO_JSON("transDuration", m_statistic.dlTransDuration, experience);
	ADD_NUMBER_PARAM_TO_JSON("pktLossRate", pktLostRate, experience);
	ADD_NUMBER_PARAM_TO_JSON("stallEventNum", stalling, experience);
	ADD_NUMBER_PARAM_TO_JSON("stallDuration", stallDuration, experience);
	ADD_NUMBER_PARAM_TO_JSON("freezEventNum", m_statistic.freezingEventNum, experience);
	ADD_NUMBER_PARAM_TO_JSON("freezDuration", m_statistic.freezingDuration, experience);
	ADD_NUMBER_PARAM_TO_JSON("loadingDelay", m_statistic.loadingDalay, experience);

	m_statistic.dlTransDuration = 0.0;
	m_statistic.freezingEventNum = 0;
	m_statistic.freezingDuration = 0.0;
	m_statistic.pktStatInfo = stat;
	return experience;
}

char* AvsRtspProcessor::alloc_ingress_data_buf(uint32_t len, uint32_t& ulBufLen) {
	if (m_buffSize < len) {
		return nullptr;
	}
	ulBufLen = m_buffSize;
	return &m_mediaBuf[sizeof(MediaInfoMsg)];
}

int32_t AvsRtspProcessor::handle_ingress_status(MEDIA_STATUS_INFO statusInfo) {
	if (!m_master) {
		AS_LOG(AS_LOG_WARNING, "not specified handler when status coming.");
		return -1;
	}

	STREAM_STATUS_TYPE status = STREAM_STATUS_NONE;
	std::string result;

	switch (statusInfo.enStatus) {
	case MR_CLIENT_STATUS_CONNECTED:
		status = STREAM_STATUS_CONNECTED;
		break;

	case MR_CLIENT_CONNECT_FAILED:
		status = STREAM_CONNECT_FAILED;
		break;

	case MR_CLIENT_MEDIA_ATTRIBUTE: {
		if (!m_ingress || !(m_formatCtx = m_ingress->get_format_context())) {
			return AS_ERROR_CODE_FAIL;
		}

		MK_Stream* stream = nullptr;
		if (m_formatCtx->video_stream >= 0) {
			stream = m_formatCtx->streams.at(m_formatCtx->video_stream);
			switch (stream->codecpar->codec_id) {
			case MK_CODEC_ID_H264:
				m_mediaInfo.video_fomrat = 0;
				break;
			case MK_CODEC_ID_HEVC:
				m_mediaInfo.video_fomrat = 1;
				break;
			case MK_CODEC_ID_MJPEG:
				m_mediaInfo.video_fomrat = 2;
				break;
			default:
				m_mediaInfo.video_fomrat = UINT8_MAX;
				break;
			}
		}

		if (m_formatCtx->audio_stream >= 0) {
			stream = m_formatCtx->streams.at(m_formatCtx->audio_stream);
			switch (stream->codecpar->codec_id) {
			case MK_CODEC_ID_PCM_ALAW:
				m_mediaInfo.audio_format = 0;
				break;
			case MK_CODEC_ID_PCM_MULAW:
				m_mediaInfo.audio_format = 1;
				break;
			case MK_CODEC_ID_AAC:
				m_mediaInfo.audio_format = 2;
				break;
			default:
				m_mediaInfo.audio_format = UINT8_MAX;
				break;
			}
		}
	}
		if (setInternalParser() < 0) {
			if (m_parser) {
				m_parser->close();
			}
			status = (STREAM_STATUS_TYPE)OPEN_DECODER_ERROR;
		}
		break;

	case MK_CLENT_PT_ADAPT:
		if (!m_mediaInited) {
			return 0;
		}
		m_master->adaptMediaAttribute(m_formatCtx);
		break;

	case MR_CLIENT_STATUS_HANDSHAKE:
		status = STREAM_STATUS_SETUP; 
		m_status = kPlaying;
		break;

	case MR_CLIENT_STATUS_TEARDOWN:
		status = STREAM_STATUS_TEARDOWN;
		break;

	case MR_CLIENT_SETUP_TIMEOUT:
		AS_LOG(AS_LOG_INFO, "player[%ld] setup timeout.", m_master->id);
		status = STREAM_STATUS_SETUP_TIMEOUT;
		break;
	case MR_CLIENT_STATUS_TIMEOUT:
		if (kPlaying == m_status) {
			double timeoutThreshold = m_master->speed / 0.05;
			if (timeoutThreshold > 1.0 || timeoutThreshold < -1.0) {
				status = STREAM_STATUS_TIMEOUT;
				AS_LOG(AS_LOG_INFO, "player[%ld] recv media data timeout.", m_master->id);
			}
		}
		break;

	case MR_CLIENT_STATUS_RECV_FAIL:
		status = STREAM_STATUS_CONN_ERROR;
		break;
	case MR_CLIENT_STATUS_CONN_CLOSE:
		status = STREAM_STATUS_CONN_CLOSE;
		if (TYPE_DOWNLOAD_START == m_master->type) {
			if (m_parser) {
				m_parser->close();
			}
			m_master->handleClose();
			m_status = kEos;
			return AS_ERROR_CODE_OK;	
		}
		break;

	case MR_CLIENT_STATUS_SRV_ERROR:
		status = STREAM_STATUS_SRV_ERROR;
		result = std::to_string(statusInfo.errCode);
		break;

	case MR_CLIENT_STATUS_EOS:
		status = STREAM_STATUS_EOS;
		if (TYPE_DOWNLOAD_START == m_master->type) {
			if (m_parser) {
				m_parser->close();
			}
			m_master->handleClose();
			m_status = kEos;
			return AS_ERROR_CODE_OK;
		}
		break;

	case MR_CLIENT_STATUS_PAUSE:
		status = STREAM_STATUS_PAUSE_RESP;
		m_status = kPaused;
		break;

	case MR_CLIENT_STATUS_RUNNING:
		if (SeekState::kWaitReply == m_seekState) {
			m_seekState = SeekState::kReplyArrive;
		}
		if (kStepWaitForReverse == m_stepState) {
			m_stepState = kStepWaitForPause;
			m_master->clearBuffer();
		}
		m_status = kPlaying;
		break;

	case MR_CLIENT_FRAG_MISTACH:
		status = STREAM_MULTI_FRAGS_MISMATCH;
		break;
	default:
		return AS_ERROR_CODE_FAIL;
	};

	if (STREAM_STATUS_NONE != status) {
		m_master->sendStatus((long)status, result);
	}
	return 0;
}

int32_t AvsRtspProcessor::handle_ingress_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len) {
	if (!m_formatCtx || !m_parser) {
		return AS_ERROR_CODE_FAIL;
	}

	if (kStepWaitForReverse == m_stepState) {
		return AS_ERROR_CODE_FAIL;
	}

	static const int kMeanBitrateInterval = 5000;
	int ret = 0;
	statisMediaSize(m_formatCtx->video_stream == dataInfo->stream_index, len);
	int64_t curMs = getNowMs();
	if (curMs - m_curBitRateStartMs > 1000) {
		freshBitRate(true, curMs - m_curBitRateStartMs);
		m_curBitRateStartMs = curMs;
	}
	if (curMs - m_meanBitRateStartMs > kMeanBitrateInterval) {
		freshBitRate(false, curMs - m_meanBitRateStartMs);
		m_meanBitRateStartMs = curMs;
	}

	double delta = 0.0;
	int64_t pts = dataInfo->pts, ptsDelta;
	static const int kPtsRate = 90;
	
	//Video
	if (m_formatCtx->video_stream == dataInfo->stream_index) {
		if (!m_frameArrived) {
			if (m_mediaInited) {
				m_statistic.loadingDalay = getNowMs() - m_loadStartMs;
				m_frameArrived = true;
				AS_LOG(AS_LOG_NOTICE, "biz[%ld] recv key until %d packet has waited.", m_master->id, m_pktsWaited);
				m_pktsWaited = 0;
			}
			else {
				++m_pktsWaited;
				return mk_recv_next_media_data(client);
			}
		}
		else {
			delta = 0.0;
			ptsDelta = pts - m_lastVideoTS;
		
			if (ptsDelta) {
				m_statistic.dlTransDuration += (m_stdFps ? 1.0 / m_stdFps : 0.04);
			}

			if (kStepWaitForPause == m_stepState) {
				if (((ptsDelta > 0) && (m_stepDirect < 0)) ||
					((ptsDelta < 0) && (m_stepDirect > 0)))
					return mk_recv_next_media_data(client);
			}

			//todo:seekû����Ӧ��ô����? 
			/*if (SeekState::kWaitReply == m_seekState) {
				return mk_recv_next_media_data(client);
			}*/

			const int kSeekEffectVal = 2000;
			if ((SeekState::kReplyArrive == m_seekState) && (abs(ptsDelta) > kSeekEffectVal)) {
				m_videoTS = m_seekTimer;
				m_audioTs = m_seekTimer;
				m_seekState = SeekState::kWaitReq;
			}
			else {
				double duration = m_stdFps ? 1.0 / m_stdFps : 0.04;
				if (abs(ptsDelta) > UINT32_MAX / (kPtsRate * 2)) {
					//ptsDelta > 0 : 0->MAX
					delta = ptsDelta > 0 ? -duration : duration;
				}
				else {
					delta = ptsDelta / 1000.0;
				}
				m_videoTS += delta;

				if (abs(delta) >= 2 * duration) {
					m_statistic.freezingEventNum++;
					m_statistic.freezingDuration += (delta - duration);
				}

				if (m_reverse) {
					if (delta == 0) {
						//AS_LOG(AS_LOG_WARNING, "delta equal 0.0");
					}
					if (delta > 0.0) {
						return 0;
					}
				}
			}
		}

		m_lastVideoTS = pts;

		auto stream = m_formatCtx->streams[m_formatCtx->video_stream];
		if (!stream) {
			AS_LOG(AS_LOG_WARNING, "video stream isn't available.");
			return -1;
		}
		
		/*
		AVRational tb{ stream->time_base.num, stream->time_base.den };
		dataInfo->pts = av_rescale_q(m_curInputTimer * 1000, { 1, 1000 }, tb);
		if (dataInfo->ntp > 0.0) {
			dataInfo->pts = dataInfo->ntp * 1000;
		}
		else {
			dataInfo->pts = m_videoTS * 1000;
		}
		*/
		dataInfo->pts = m_videoTS * 1000;

		if (stream && stream->codecpar && (stream->codecpar->video_fps > 0.00001)) {
			m_stdFps = (int)stream->codecpar->video_fps;
		}
		else if (!m_stdFps && stream && (stream->interval > 0)) {
			static const int kMaxHz = 50;
			int hz = (int)(stream->time_base.den / stream->interval);
			if (hz < kMaxHz) {
				m_stdFps = hz;
			}
		}

#ifdef  VIDEO_DUMP_FILE
		if (!m_videoFile[dataInfo->fragment].is_open()) {
			char dump[128] = { 0 };
			sprintf_s(dump, "dump_%ld_%d.%s", m_master->id, dataInfo->fragment, MK_CODEC_ID_H264 == dataInfo->codec_id ? "h264" : "h265");
			m_videoFile[dataInfo->fragment].open(dump, ofstream::out | ios::binary);
		}
#endif //  VIDEO_DUMP_FILE

		if (dataInfo->is_key) {
            auto configFrames = stream->track->getConfigFrames();
			int len1 = 0;
			for (auto& config : configFrames) {
				if (config->m_inserted) {
					continue;
				}
				if (config->m_dts == pts || UINT64_MAX == config->m_dts) {
					memcpy(m_configData + len1, "\x00\x00\x00\x01", 4);
					len1 += 4;
					memcpy(m_configData + len1, config->m_buffer.data(), config->m_buffer.size());
					len1 += config->m_buffer.size();

					if (len1 > 0) {
	#ifdef  VIDEO_DUMP_FILE
						m_videoFile[dataInfo->fragment].write((const char*)m_configData, len1);
	#endif //  VIDEO_DUMP_FILE

						m_parser->praseConfigData(m_configData, len1, dataInfo->pts, dataInfo->pts);
					}
					len1 = 0;

					if (config->m_dts != UINT64_MAX) {
						config->m_inserted = true;
					}
				}
				else {
					AS_LOG(AS_LOG_WARNING, "drop unfit config.");
				}
			}
			/*if (len1 > 0) {
				m_parser->praseConfigData(m_configData, len1, dataInfo->pts, dataInfo->pts);
			}*/
		}
#ifdef  VIDEO_DUMP_FILE
		m_videoFile->write((const char*)(&m_mediaBuf[sizeof(MediaInfoMsg)] + dataInfo->offset), len);
#endif //  VIDEO_DUMP_FILE
	}
	//Audio
	else {
		if (!m_frameArrived) {
			return mk_recv_next_media_data(client);
		}

		if (0 == m_lastAudioTs) {
			m_lastAudioTs = pts;
		}
		ptsDelta = pts - m_lastAudioTs;
		double duration = m_stdFps ? 1.0 / m_stdFps : 0.04;
		if (abs(ptsDelta) > UINT32_MAX / (kPtsRate * 2)) {
			//ptsDelta > 0 : 0->MAX
			delta = ptsDelta > 0 ? -duration : duration;
		}
		else {
			delta = ptsDelta / 1000.0;
		}
		m_audioTs += delta;
		m_lastAudioTs = pts;
		dataInfo->pts = m_audioTs * 1000;

#ifdef  AUDIO_DUMP_FILE
		if (!m_auidoFile.is_open() && (void*)INVALID_WND == m_bizParams.WindowsHandle) {
			char audio[128] = { 0 };
			sprintf_s(audio, "dump_%ld.g711", m_bizParams.BizId);
			m_auidoFile.open(audio, ofstream::out | ios::binary);
		}
		m_auidoFile.write((const char*)(&m_szBuf[sizeof(MediaInfoMsg)] + dataInfo->offset), len);
#endif //  AUDIO_DUMP_FILE
	}

	auto header_size = sizeof(MediaInfoMsg);
	char* rtpData = m_mediaBuf;
	if (dataInfo->offset) {
		rtpData = m_mediaBuf + header_size + dataInfo->offset;
	}
	auto media_msg = (MediaInfoMsg*)rtpData;
	media_msg->dataInfo = *dataInfo;
	media_msg->dataSize = len;

	if (!m_reverse) {
		m_parser->parseMediaData(media_msg);
	}
	else {
		auto frame = string(header_size + media_msg->dataSize, '\0');
		auto header = (MediaInfoMsg*)frame.data();

		header->dataInfo = media_msg->dataInfo;
		header->dataSize = media_msg->dataSize;
		memcpy((void*)(frame.data() +  header_size), rtpData + header_size, media_msg->dataSize);
		m_reversalGop.emplace_front(frame);

		if (header->dataInfo.is_key) {
			for (auto& pkt : m_reversalGop) {
				m_parser->parseMediaData((MediaInfoMsg*)pkt.data());
			}
			m_reversalGop.clear();
		}
	}
	return mk_recv_next_media_data(client);
}

void AvsRtspProcessor::update(NotiData notice) {
	switch (notice.type) {
	case NotiData::MEDIA: {
		m_mediaInfo.res_width = notice.width;
		m_mediaInfo.res_height = notice.height;
		m_mediaInfo.fps = notice.fps;
		if (m_formatCtx && (m_formatCtx->video_stream >= 0)) {
			auto stream = m_formatCtx->streams[m_formatCtx->video_stream];
			stream->codecpar->width = notice.width;
			stream->codecpar->height = notice.height;
		}
	}
		break;
	default:
		break;
	}
}

int AvsRtspProcessor::setInternalParser() {
	if (!m_formatCtx) {
		return AVERROR(EINVAL);
	}

	if (!m_parser && !(m_parser = AS_NEW(m_parser))) {
		return AVERROR(ENOMEM);
	}

	m_parser->close();
	int ret = m_parser->init(m_formatCtx);
	m_parser->addTask(m_master);
	if (ret >= 0) {
		if ((ret = m_master->setMediaAttribute(m_formatCtx)) >= 0) {
			//m_master->addObserver(this);
			m_mediaInited = true;
		}
	}
	return ret;
}

void AvsRtspProcessor::statisMediaSize(bool isVideo, uint32_t uiFrameSize) {
	m_statisMediaSize[0 + !isVideo] += uiFrameSize;
	m_statisMediaSize[2 + !isVideo] += uiFrameSize;
}

void AvsRtspProcessor::freshBitRate(bool isCurBitRate, uint32_t intervalMs) {
	double videoRateKSize = (double)m_statisMediaSize[0 + 2 * isCurBitRate] / 1024.0;
	double audioRateKSize = (double)m_statisMediaSize[1 + 2 * isCurBitRate] / 1024.0;

	if (isCurBitRate) {
		m_mediaInfo.video_cur_bitrate = videoRateKSize / (intervalMs / 1000);
		m_mediaInfo.audio_cur_bitrate = audioRateKSize / (intervalMs / 1000);
		/*AS_LOG(AS_LOG_DEBUG, "video_cur_bitrate:%d,audio_cur_bitrate:%d",
			m_mediaInfo.video_cur_bitrate, m_mediaInfo.audio_cur_bitrate);*/
	}
	else {
		m_mediaInfo.video_mean_bitrate = videoRateKSize / (intervalMs / 1000);
		m_mediaInfo.audio_mean_bitrate = audioRateKSize / (intervalMs / 1000);
		/*AS_LOG(AS_LOG_DEBUG, "video_mean_bitrate:%d,audio_mean_bitrate:%d",
			m_mediaInfo.video_mean_bitrate, m_mediaInfo.audio_mean_bitrate);*/
	}
	m_statisMediaSize[0 + 2 * isCurBitRate] = 0;
	m_statisMediaSize[1 + 2 * isCurBitRate] = 0;
}

void AvsRtspProcessor::stepPause() {
	if (kPaused != m_status) {
		if (m_ingress && !m_master->realtime) {
			m_ingress->pause_ingress();
		}
	}
	m_stepState = kStepEnd;
	m_master->sendStatus(STREAM_STATUS_PAUSE, "");
}


