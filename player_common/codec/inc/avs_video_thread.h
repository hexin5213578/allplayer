#pragma once

#include <memory>

#include "avs_decdoe_thread.h"
#include "avs_watermark.h"

class AvsVideoView;
class AvsVideoFilter;

class AvsVideoThread : public AvsDecodeThread
{
public:
	AvsVideoThread(VideoState* ic);
	virtual ~AvsVideoThread();

	//clear: 是否清除缓存
	void pause(bool isPause, bool clear = false) override;
	void stopRun() override;
	void restart() override;
	void clear() override;

	int probeHwAcc(MK_Stream* stream);
	int setDecodePara(AVStream* stream, bool isHW);
	
	int doTask(AVPacket*) override;
	void pushPacket(AVPacket* pkt) override;

	bool snapPicture(std::string filePath, uint8_t format);
	void captureFrames(std::string filePath, uint16_t count);

	int adjustPictureParams(std::string& params);
	std::string getPictureParams();

	int32_t getStepFrameNB();
	void step2NextFrame(int8_t stepDirect);
	void exitStep(bool isSeek);

protected:
	int decoding() override;
	int processFrame(AVFrame* frame);

private:
	int getVideoFrame();
	int applyPictureFilter(AVFrame* frame);
	int queueFrame(AVFrame* frame);
	void implCapture();

	int putFrame2Render(MyFrame::Ptr frame, int block, int timeout = 3000);

protected:
	AVFrame*				m_hwFrame = nullptr;
    
private:
	uint32_t				m_ulWidth = 0;
	uint32_t				m_ulHeight = 0;

	AvsVideoFilter*			m_picFilter = nullptr;			//图像滤镜
	std::mutex				m_picFilterMtx;

	ACWaterMark*			m_waterMark = nullptr;			//水印滤镜
	std::mutex				m_waterMutex;

protected:
	std::string				m_capturePath;
	uint16_t				m_captureCount = 0;
	uint16_t				m_captured = 0;

	FrameGops::Ptr			m_gops;
};
