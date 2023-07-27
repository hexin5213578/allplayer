#pragma once
#include "avs_decdoe_thread.h"
#include "avs_audio_play.h"

class AvsAudioFilter;
class AvsResample;

class AvsAudioThread : public AvsDecodeThread, public Task
{
public:
	AvsAudioThread(VideoState* ic);
	virtual ~AvsAudioThread() = default;

	void setVolume(int16_t volume);

	void pause(bool isPause, bool needClear) override;

	int doTask(AVPacket* pkt) override;

	void pushPacket(AVPacket* pkt) override;

	template <typename T>
	int initAudioPara(T* params, int16_t vol) {
		AvsAudioPlay::GetInstance()->close();
		if (!!(m_audioPlayer = AvsAudioPlay::CreateInstance())) {
			m_audioPlayer->setVideoState(m_videoState);
			if (m_audioPlayer->initAudioPara(params)) {
				m_audioPlayer->setVolume(vol);
			}
		}
		return -1;
	}

	void stopRun() override;
	int resample(AVFrame* frame, int & resampled_data_size, uint8_t*** converted_input_samples);
	int initResample(int64_t layout, AVSampleFormat format, int sampleRate, int nbSamples);

protected:
	int decoding() override;
	virtual int processFrame(AVFrame* frame) override;

private:
	int16_t				m_volume;
	AvsAudioFilter*		m_filter;
	AvsAudioPlay*		m_audioPlayer;
	AvsResample*			m_resample;
};
