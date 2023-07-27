#pragma once

extern "C" 
{
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}

class AvsResample
{
public:
	AvsResample();
	virtual ~AvsResample();

	int setParameters(int64_t layout, AVSampleFormat format, int sampleRate, int nbSamples);

	int resample(AVFrame* srcFrame, uint8_t*** converted_input_samples, int* size = nullptr);

private:
	uint64_t		m_dstLayout;
	int				m_dstChannels;
	AVSampleFormat	m_dstSampleFormat;
	int				m_dstSampleRate;
	int				m_dstNbSampls;
	SwrContext*		m_swrCtx;
};
