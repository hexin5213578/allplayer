#pragma once

#include "avs_player.h"
#include "avs_rtsp_processor.h"
#include "view_factory.h"

class AvsMuxThread;

class AvsRtspBasePlayer : public AvsPlayer {
public:
	AvsRtspBasePlayer(BizParams& p) : AvsPlayer(p) {
		this->play_mode = kRtsp;
		this->pre_cache = 1;
	}
	virtual ~AvsRtspBasePlayer() = default;
	
	int play() override {
		if (m_hwnd && m_hwnd != (void*)INVALID_WND) {
			m_view = std::shared_ptr<AvsVideoView>(ViewFactory::getInstance()->createVideoView(m_hwnd), [](AvsVideoView* view) {
                if(view) {
                    ViewFactory::getInstance()->detroyViewView(view->getHwnd());
                }
            });
			return !m_view;
		}
		return 0;
	}

	void pause(bool pause, bool clear) override;

	int doTask(AVPacket* pkt) override;

protected:
	int configVideo(MK_Stream* stream) override;

	int configAudio(MK_Stream* stream) override;

	AvsVideoView::Ptr	m_view;
};

class AvsRtspPlayer : public AvsRtspBasePlayer, public AvsRtspProcessor {
public:
	AvsRtspPlayer(BizParams& bizParams);
	virtual ~AvsRtspPlayer() = default;

	int play() override;

	void stop() override {
		stopCapture();
		stop_1();
		AvsPlayer::stop();
	}

	int setUrl(BizParams params) override;
	
	void pause(bool pause, bool clear = false) override;

	int vcrControl(double start, double scale, int option) override;

	void startCapture(BizParams capture) override;
	void stopCapture() override;

	void setSkipNokey(bool skip) override {
		AvsPlayer::setSkipNokey(skip);
		if (skip) {
			m_reverse = false;
		}
	}

	std::string getMediaInfo() override{
		return getMediaInfo_1();
	}

	cJSON* getExperienceData() override {
		return getExperience_1();
	}

	void step2NextFrame(uint8_t forward) override;
	void exitStep() override;

private:
	void exitStep_2(bool isSeek);
	void step2NextFrame_2(int8_t direct);

	bool				m_reverse = false;
	std::mutex			m_recordMutex;
	AvsMuxThread*		m_recordTh = nullptr;
	bool				m_recording = false;
};

