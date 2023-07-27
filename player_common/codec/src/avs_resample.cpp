#include "avs_resample.h"

AvsResample::AvsResample()
	:m_swrCtx(nullptr) {
}

AvsResample::~AvsResample()
{
	if (m_swrCtx) {
		swr_free(&m_swrCtx);
	}
}

int AvsResample::setParameters(int64_t layout, AVSampleFormat format, int sampleRate, int nbSamples)
{
	m_dstLayout = layout;
	m_dstSampleFormat = format;
	m_dstSampleRate = sampleRate;
	m_dstNbSampls = nbSamples;
	return 0;
}

int32_t AvsResample::resample(AVFrame* srcFrame, uint8_t*** converted_input_samples, int* size)
{
	int ret = 0;
	// 处理可能缺少的声道个数和声道布局参数
	if (srcFrame->channels == 0 && srcFrame->channel_layout == 0) {
		//use default channel value
	}
	else if (srcFrame->channels > 0 && srcFrame->channel_layout == 0)  {
		srcFrame->channel_layout = av_get_default_channel_layout(srcFrame->channels);
	}
	else if (srcFrame->channels == 0 && srcFrame->channel_layout > 0) {
		srcFrame->channels = av_get_channel_layout_nb_channels(srcFrame->channel_layout);
	}

	m_dstNbSampls = av_rescale_rnd(srcFrame->nb_samples, m_dstSampleRate, srcFrame->sample_rate, AV_ROUND_UP);

	do {
		//初始化转换上下文
		m_swrCtx = swr_alloc_set_opts(m_swrCtx, m_dstLayout, m_dstSampleFormat, m_dstSampleRate,
			srcFrame->channel_layout, static_cast<AVSampleFormat>(srcFrame->format), srcFrame->sample_rate,
			0, nullptr);

		if (!m_swrCtx) {
			ret = AVERROR(ENOMEM);
			break;
		}

		if ((ret = swr_init(m_swrCtx)) < 0)
			break;

		m_dstChannels = av_get_channel_layout_nb_channels(m_dstLayout);

		if (!(*converted_input_samples = (uint8_t**)calloc(m_dstChannels, sizeof(**converted_input_samples)))) {
			ret = AVERROR(ENOMEM);
			break;
		}
		
		
		if ((ret = av_samples_alloc(*converted_input_samples, nullptr, m_dstChannels, m_dstNbSampls,
			m_dstSampleFormat, 0)) < 0)
		{
			av_freep(&(*converted_input_samples)[0]);
			free(*converted_input_samples);
			return ret;
		}

		//完成音频帧转换
		ret = swr_convert(m_swrCtx, *converted_input_samples, srcFrame->nb_samples,
			(const uint8_t**)srcFrame->extended_data, srcFrame->nb_samples);

		if (ret < 0) {
			break;
		}
	} while (0);

	if (ret < 0) {
		swr_free(&m_swrCtx);
		char buf[512] = { 0 };
		av_strerror(ret, buf, sizeof(buf) -1);
	}
	if (size) {
		*size = ret * m_dstChannels * av_get_bytes_per_sample(m_dstSampleFormat);
	}
	return ret;
}
