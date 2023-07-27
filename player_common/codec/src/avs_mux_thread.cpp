#include "avs_mux_thread.h"
#include "avs_muxer.h"
#include "crypto_muxer.h"

static const int kMsPerSecond = 1000;
static const int kSecondsPerMin = 60;
const int kCapcityUnits = 1024;
const int kWaitMs = 2;

AvsMuxThread::AvsMuxThread(BizParams& p) : AvsPlayer(p) {
	if (FileDumpSt::kAVS == p.FileDump.fileFormat) {
		m_forward = true;
	}
}

AvsMuxThread::~AvsMuxThread() {
	stop();
}

int AvsMuxThread::play() {
	if (init()) {
		if (TYPE_DOWNLOAD_START == this->type) {
			this->init_1(this);
			this->start_1();
		}
		else {
			if (!getStarted()) {
				startRun();
			}
		}
		return 0;
	}
	return -1;
}

void AvsMuxThread::pause(bool pause, bool clear) {
	if (this->paused == pause) {
		return;
	}

	if (this->paused) {
		this->resume_1();
	}
	else {
		this->pause_1();
	}
	this->paused = !this->paused;
}

int AvsMuxThread::setMediaAttribute(MK_Format_Contex* format) {
	if (!format) {
		return AVERROR(EINVAL);
	}
	m_format = format;
	if (!getStarted()) {
		startRun();
	}
	return 0;
}

int AvsMuxThread::doTask(AVPacket* pkt) {
	m_pkts.push(pkt);
	return 0;
}

bool AvsMuxThread::init() {
	m_bizParams.vcrSt.scale = m_bizParams.FileDump.scale;
	m_dumpSt = m_bizParams.FileDump;
	m_szCameraId = m_bizParams.CameraId;
	if (FileDumpSt::kMP4 == m_dumpSt.fileFormat) {
		m_muxer = new AvsMuxer(id, type);
	}
	else if (FileDumpSt::kAVS == m_dumpSt.fileFormat) {
		m_muxer = new CryptoMuxer(id, type, m_dumpSt);
	}
	else {
		return false;
	}
	return nullptr != m_muxer;
}

void AvsMuxThread::stopRun()
{
	AvsThreadBase::stopRun();
	wait();

	releaseMuxer();
	while (m_pkts.size()) {
		auto pkt = m_pkts.pop();
		av_packet_free(&pkt);
	}
	sendStatus(isDownloadBiz() ? DOWNLOAD_STOP : RECORD_STOP, m_output);
}

void AvsMuxThread::handleClose() {
	AVPacket pkt1, * pkt = &pkt1;
	av_init_packet(pkt);
	pkt->data = (uint8_t*)pkt;
	pkt->size = 0;
	m_pkts.push(pkt);
	av_packet_unref(pkt);
}

int AvsMuxThread::vcrControl(double start, double scale, int option)
{
	return vcrControl_1(start, scale, option);
}

void AvsMuxThread::mainProcess() {
	if (!m_format || m_format->video_stream < 0 || !m_muxer) {
		return;
	}
	m_output = genFileName();
	MK_Stream* vid_st = m_format->streams.at(m_format->video_stream);
	if (!vid_st) {
		sendStatus(isDownloadBiz() ? DOWNLOAD_INIT_ERROR : RECORD_INIT_ERROR, "download init failed !");
		return;
	}
	auto codecpar = vid_st->codecpar;
	int ret = EAGAIN;

	double time = getRelativeTime();
	double update_time = 0.0;
	size_t bytes = 0;    //统计码流

	while (!m_bIsExit) {
		if (EAGAIN == ret) {
			int res = codecpar->width * codecpar->height;
			if (res > 0) {
				if (m_muxer->init(m_output, m_format) < 0) {
					sendStatus(isDownloadBiz() ? DOWNLOAD_INIT_ERROR : RECORD_INIT_ERROR, "download init failed !");
					return;
				}
				else {
					sendStatus(isDownloadBiz() ? DOWNLOAD_START : RECORD_START, "");
					ret = 0;
				}
			}
			else {
				AVSleep(0.01);
				continue;
			}
		}

		AVPacket* pkt = m_pkts.pop();
		if (!pkt) {
			AVSleep(kWaitMs / 1000.0);
			continue;
		}

		if ((0 == pkt->size) && ((void*)pkt->data == pkt)) {
			releaseMuxer();
			sendStatus(STREAM_STATUS_IO_FINISH, "");
			if (m_pkts.size() > 0) {
				AS_LOG(AS_LOG_WARNING, "remain packets in cache when arriving at eos.");
			}
			break;
		}
		
		auto origin_pts = pkt->pts;
		if (UINT64_MAX == m_lastPostData) {
			if (AV_NOPTS_VALUE == origin_pts) {
				continue;
			}
			m_lastRecordPts = origin_pts;
		}

		if ((0 == pkt->stream_index) && (AV_NOPTS_VALUE != origin_pts)) {
			uint64_t curSec = pkt->pts / kMsPerSecond;
			if (isDownloadBiz() && (curSec != m_lastPostData)) {
				sendData(curSec, 0);
			}
			m_lastPostData = curSec;
			pkt->pts -= m_lastRecordPts;
		}

		if (!m_waitedKeyFrame && (MEDIA_TYPE_VIDEO != pkt->stream_index || !(pkt->flags & AV_PKT_FLAG_KEY))) {
			av_packet_free(&pkt);
			continue;
		}
		
		m_waitedKeyFrame = true;

		if (m_muxer) {
			int packSize = pkt->size;
			if (m_muxer->do_egress(pkt) < 0) {
				sendStatus(FILE_WRITE_ERROR, m_output);
				AS_LOG(AS_LOG_WARNING, "biz[%ld] do remux faild, exit thread.", id);
				break;
			}
		
			m_dumpedSize += packSize;
			bytes += packSize;
			
			if ((0 == pkt->stream_index) && packSize) {
				checkNeedCutting(origin_pts);
			}
		}

		if (isDownloadBiz()) {
			update_time = getRelativeTime();
			double dur = update_time - time;
			if (dur >= 1.0) {
				std::string kbps = std::to_string(bytes / dur / 1000.0);
				sendStatus(DOWNLOAD_KBYTERATE, kbps);
				time = update_time;
				bytes = 0;
			}
		}

		av_packet_free(&pkt);
	}
}

int AvsMuxThread::releaseMuxer() {
	if (m_muxer) {
		bool remuxing = m_muxer->remuxing();
		auto ret = m_muxer->stop();
		if (remuxing) {
			if (!ret) {
				sendStatus(FILE_FRAGMENT, m_output);
			}
			else {
				sendStatus(FILE_WRITE_ERROR, m_output);
			}
		}
	}
	AS_DELETE(m_muxer);
	return 0;
}

std::string AvsMuxThread::genFileName()
{
	switch (m_dumpSt.fileFormat) {
	case FileDumpSt::kMP4:
		return getMP4FileName();
	case FileDumpSt::kAVS:
		return getAVSFileName();
	default:
		break;
	}
	return "";
}

std::string AvsMuxThread::getMP4FileName()
{
	++m_currentFileNum;
	//uint64_t ulCurTime; //= GetTickCount64();
	time_t curTime = getTimeStamp();
	//auto strCurTime = std::ctime(&curTime);
	string szCurTime, szCurGileNum;
	auto threadId = std::this_thread::get_id();
	std::string fileName = m_dumpSt.filePath;
	fileName += m_szCameraId;
	fileName += "_";
	fileName += std::to_string(curTime);
	fileName += "_";
	fileName += std::to_string(id);
	fileName += "_";
	fileName += std::to_string(m_currentFileNum);
	fileName += ".mp4";
	return fileName;
}

std::string AvsMuxThread::getAVSFileName()
{
	++m_currentFileNum;
	time_t curTime = getTimeStamp();
	//auto strCurTime = std::ctime(&curTime);
	string szCurTime, szCurGileNum;
	std::string fileName = m_dumpSt.filePath;
	fileName += m_szCameraId;
	fileName += "_";
	fileName += std::to_string(curTime);
	fileName += "_";
	fileName += std::to_string(id);
	fileName += "_";
	fileName += std::to_string(m_currentFileNum);
	fileName += ".avs";
	return fileName;
}

int32_t AvsMuxThread::checkNeedCutting(uint64_t ulCurPts)
{
	bool needCut = false;
	switch (m_dumpSt.cutFormat) {
		case 0: {
			int64_t cutDuration = m_dumpSt.cutDuration * kSecondsPerMin * kMsPerSecond;
			auto muxerDuration = m_muxer->getDuration();
			#ifdef _DEBUG
				int64_t rtpDuraion = ulCurPts - m_lastRecordPts;
				if (rtpDuraion != muxerDuration) {
					AS_LOG(AS_LOG_INFO, "rtp dur = %ld, muxer's dur = %ld.", rtpDuraion, muxerDuration);
				}
			#endif	//_DEBUG
				if (muxerDuration >= cutDuration) {
					needCut = true;
				}
			}
			break;
		case 1: {
			uint64_t cutSizeBytes = (uint64_t)m_dumpSt.cutSize * kCapcityUnits * kCapcityUnits;
			if (m_muxer->getFileSize() >= cutSizeBytes) {
				needCut = true;
				AS_LOG(AS_LOG_INFO, "file dumpsize %lld, filesize %lld.", m_dumpedSize, m_muxer->getFileSize());
			}
			break;
		}
		default:
			break;
	}

	int32_t ret = 0;
	if (needCut) {
		if (AV_NOPTS_VALUE != ulCurPts) {
			m_lastRecordPts = ulCurPts;
		}
		else {
			m_lastRecordPts = m_lastRecordPts + m_muxer->getDuration();
		}

		if (AS_ERROR_CODE_OK == m_muxer->write_trailer()) {
			sendStatus(FILE_FRAGMENT, m_output);
		}
		else {
			sendStatus(FILE_WRITE_ERROR, m_output);
		}
		m_output = genFileName();
		m_dumpedSize = 0; 
		m_waitedKeyFrame = m_muxer->waitForNextKey()? false : true;
		if ((ret = m_muxer->init(m_output, m_format)) < 0) {
			sendStatus(isDownloadBiz() ? DOWNLOAD_INIT_ERROR : RECORD_INIT_ERROR, "download init failed !");
		}
		else {
			sendStatus(isDownloadBiz() ? DOWNLOAD_START : RECORD_START, "");
		}
	}
	return needCut;
}
