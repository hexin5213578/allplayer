#include "avs_audio_filter.h"
#include "as_log.h"

extern "C" {
#include "libavutil/opt.h"
}

#define FF_AFILTER_LEN	256

int AvsAudioFilter::parse_filter_params(std::string & params)
{
	m_filterDescr = params;
	m_filterChanged = true;
	return 0;
}

bool AvsAudioFilter::need_reconfig(AVFrame* frame)
{
	return m_graph && 
		(m_filterChanged || (m_sampleRate != frame->sample_rate) || (m_lastFormat != frame->format) || (m_channels != frame->channels));
}

int AvsAudioFilter::configure_filters(const char* filters, AVFrame* frame)
{
	static const enum AVSampleFormat sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };

	if (need_reconfig(frame)) {
		int ret;
		char asrc_args[FF_AFILTER_LEN] = { 0 };
		AVFilterContext* filt_asrc = NULL, * filt_asink = NULL;
		
		ret = snprintf(asrc_args, sizeof(asrc_args),
			"sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
			frame->sample_rate, av_get_sample_fmt_name(AVSampleFormat(frame->format)), frame->channels, 
			1, frame->sample_rate);

		if (frame->channel_layout)
			snprintf(asrc_args + ret, sizeof(asrc_args) - ret, ":channel_layout=0x%llx", frame->channel_layout);

		char err_buf[128] = { 0 };
		do {
			if ((ret = avfilter_graph_create_filter(&filt_asrc, avfilter_get_by_name("abuffer"), "acplay_abuffer", asrc_args, NULL, m_graph)) < 0)
				break;

			ret = avfilter_graph_create_filter(&filt_asink, avfilter_get_by_name("abuffersink"), "acplay_abuffersink", NULL, NULL, m_graph);
			if (ret < 0) {
				break;
			}

			if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) < 0) {
				break;
			}

			if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0) {
				break;
			}

			if ((ret = configure_filtergraph(filters, filt_asrc, filt_asink)) < 0) {
				break;
			}

		} while (0);

		if (ret < 0) {
			av_strerror(ret, err_buf, sizeof(err_buf) - 1);
			AS_LOG(AS_LOG_WARNING, "biz[%ld] configure auido filters err: %s.", m_bizId, err_buf);
			return ret;
		}

		m_buffersrc_ctx = filt_asrc;
		m_buffersink_ctx = filt_asink;

		m_sampleRate = frame->sample_rate;
		m_lastFormat = frame->format;
		m_channels = frame->channels;
		return ret;
	}
	return -1;
}