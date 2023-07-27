#pragma once
#include "avs_filter.h"

class AvsAudioFilter : public AvsFilter
{
public:
	AvsAudioFilter() 
		:m_scale(1.0),
		m_sampleRate(0),
		m_channels(0)
		{ }
	virtual ~AvsAudioFilter() { }

	int parse_filter_params(std::string& params) override;

protected:
	bool need_reconfig(AVFrame* frame) override;

	int configure_filters(const char* filters, AVFrame* frame) override;

private:
	double		m_scale;
	int			m_sampleRate;
	int			m_channels;
};

