#ifndef __AVS_PLAYER_H__
#define __AVS_PLAYER_H__

#include <string.h>

#include "avs_player_common.h"
#include "avs_video_state.h"

class AvsVideoThread;
class AvsAudioThread;
class AvsRefreshLoop;

class AvsPlayer : public VideoState, public Task
{
public:
	AvsPlayer(BizParams& p) : VideoState(p.BizId, p.BizType), m_bizParams(p) { 
		m_hwnd = p.WindowsHandle;
		m_hardwareAcc = !!p.DecodeType;
		this->audio_volume = p.VolumeControl;
	}
	virtual ~AvsPlayer() = default;

	virtual int play() = 0;

	virtual void stop();
	
	virtual int setUrl(BizParams params) { return 0; }
	
	virtual void pause(bool pause, bool clear = false) = 0;
	
	virtual int vcrControl(double start, double scale, int option) { return 0; }
	
	virtual void seek(double position) { }

	const BizParams& getBizParams() { return m_bizParams; }

	void setVolume(int16_t volume);
	void setAudioChannel(bool start);

	void setScale(double scale);

	void setHwAcc(bool hw) { m_hardwareAcc = hw; }
	
	void surfaceDiff(int width, int height);

	virtual int setMediaAttribute(MK_Format_Contex* format);
	int adaptMediaAttribute(MK_Format_Contex* fmt);

	virtual std::string getPlayInfo() { return ""; }
	virtual std::string getMediaInfo() { return " "; }
	virtual cJSON* getExperienceData() { return nullptr; }

	int adjustPictureParams(std::string pic);
	std::string getPictureParams();

	int snapPicture(std::string filePath, uint8_t format);
	void captureFrames(std::string capturePath, uint16_t count);

	virtual void startCapture(BizParams capture) { }
	virtual void stopCapture() { }

	virtual void setSkipNokey(bool skip);

	int zoomOut(ZoomSt& zoom);
	int zoomIn();

	virtual void step2NextFrame(uint8_t forward) { }
	virtual void exitStep() { };

	void* getHwnd() { return m_bizParams.WindowsHandle; }
	vector<void*> getHwnds() { return m_bizParams.StitchInfo.HWNDs; }

	void getStallingInfo(int& stallEvent, double& stallaDuration);
	int getFps();

	virtual void handleClose() {};
	void clearBuffer();

	void clearSurface();
	void changeSurface(void* win, int width, int height);

protected:
	virtual int configVideo(MK_Stream* stream) { return 0; }
	virtual int configAudio(MK_Stream* stream) { return 0; }

protected:
	BizParams			m_bizParams;
	AvsVideoThread*		m_videoThread = nullptr;
	AvsAudioThread*		m_audioThread = nullptr;
	AvsRefreshLoop*		m_refreshLoop = nullptr;

	int					m_audioVol = 0;
	bool				m_hardwareAcc = false;
	void*				m_hwnd = nullptr;
};


#endif /* __AVS_PLAYER_H__ */
