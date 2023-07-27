#ifndef  __AVS_RTSP_PROCESSOR_H__
#define __AVS_RTSP_PROCESSOR_H__

#include "avs_rtsp_ingress.h"
#include "avs_player.h"
#include "avs_parser.h"

#include <set>
#include <memory>

#define STREAM_NB	18

struct stMediaInfo {
	uint32_t	res_width = 0;
	uint32_t	res_height = 0;
	uint32_t	fps = 0;
	uint8_t		video_fomrat;	//0-H264,1-H265,2-MJPEG
	uint8_t		audio_format;	//0-PCMA,1-PCMU,2-AAC
	uint64_t	recv_stream_kb = 0;
	uint32_t	video_cur_bitrate = 0;
	uint32_t	audio_cur_bitrate = 0;
	uint32_t	video_mean_bitrate = 0;
	uint32_t	audio_mean_bitrate = 0;
	double		packet_lost_rate = 0.0;
	std::string serverIP = "";
};

struct stExperience {
	int			loadingDalay;
	double		dlTransDuration;
	int			freezingEventNum;
	double		freezingDuration;
	RTP_PACKET_STAT_INFO pktStatInfo;
};

class AvsRtspProcessor : public avs_ingress_data, public IObserver {
	enum PlayStatus {
		kConnecting = 1,
		kPlaying = 2,
		kPaused = 3,
		kEos = 4,
		kStopped = 5,
	};

	//seek状态， 0为播放状态, 1为seek请求发出等待响应, 2为seek响应到达
	enum SeekState {
		kWaitReq = 0,
		kWaitReply = 1,
		kReplyArrive = 2,
		//TIME_ALTER	= 3,		//coiled timestamp disappear
	};

	enum StepState {
		kStepEnd = 0,
		kStepWaitForReverse,
		kStepWaitForPause,
	};

public:
	AvsRtspProcessor();
	virtual ~AvsRtspProcessor();

protected:
	int init_1(AvsPlayer* master);
	int start_1();
	void stop_1();
	int reload_1(BizParams& params);

	void addDelegate(Task* delegate) {
		if (m_parser) {
			m_parser->addTask(delegate);
		}
	}

	void removeDelegate(Task* delegate) {
		if (m_parser) {
			m_parser->deleteTask(delegate);
		}
	}

	int vcrControl_1(double start, double scale, int scaleOrSpeed);

	void pause_1();
	void resume_1();

	int step2NextFrame_1(int8_t direct);

	std::string getMediaInfo_1();
	cJSON* getExperience_1();

//----------------------------------------------------------------------------
	//分配空间接收rtsp返回的数据
	char* alloc_ingress_data_buf(uint32_t len, uint32_t& ulBufLen) override;

	int32_t handle_ingress_status(MEDIA_STATUS_INFO statusInfo) override;

	int32_t handle_ingress_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len) override;

//------------------------------------------------------------------------------------
	void update(NotiData notice) override;

protected:
	int setInternalParser();

	void exitStep_1() { 
		resume_1(); 
	}

private:
	//统计视频、音频在时间间隔内总大小
	void statisMediaSize(bool isVideo, uint32_t uiFrameSize);

	/**
	 * 刷新bitrate
	 * \param isCurBitRate  true:实时，fals:平均
	 * \param intervalMs
	 */
	void freshBitRate(bool isCurBitRate, uint32_t intervalMs);

	void stepGive() {
		if (kPaused == m_status) {
			m_ingress->vcr_control(-1, m_stepDirect);
		}
	}

	void stepPause();
	void stepResume() { resume_1(); }

protected:
	MK_Format_Contex*		m_formatCtx = nullptr;

private:
	AvsPlayer*				m_master = nullptr;
	//std::set<Task*>			m_delegates;
	PlayStatus				m_status = kStopped;
	AvsRtspIngress*			m_ingress = nullptr;
	uint8_t					m_configData[4096];
	char*					m_mediaBuf = nullptr;
	int						m_buffSize = 0;

	bool					m_mediaInited = false;
	AvsStreamParser*		m_parser = nullptr;
	stMediaInfo				m_mediaInfo;
	bool					m_frameArrived = false;

	int						m_pktsWaited = 0;
	int64_t					m_loadStartMs;						 //业务开始加载的时间
	stExperience			m_statistic;

	SeekState				m_seekState = kWaitReq;
	double					m_seekTimer = -1.0;

	StepState				m_stepState = kStepEnd;
	int8_t					m_stepDirect = 0;

	int						m_stdFps = 0;
	double					m_audioTs = 0.0;
	int64_t					m_lastAudioTs = 0;
	double					m_videoTS = 0.0;
	int64_t					m_lastVideoTS = 0;

	std::vector<uint32_t>	m_statisMediaSize;
	uint64_t				m_curBitRateStartMs;
	uint64_t				m_meanBitRateStartMs;				//average

	ofstream				m_auidoFile;
	ofstream				m_videoFile[STREAM_NB];
	bool					m_reverse = false;
	avs_toolkit::List<std::string >	m_reversalGop;
};

#endif // ! __AVS_RTSP_PROCESSOR_H__