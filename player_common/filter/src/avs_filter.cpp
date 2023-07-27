#include "avs_filter.h"
#include "as_log.h"


AvsFilter::~AvsFilter() {
	avfilter_graph_free(&m_graph);
}

bool AvsFilter::init(long bizId)
{
	avfilter_graph_free(&m_graph);
	m_graph = avfilter_graph_alloc();
	if (!m_graph) {
		AS_LOG(AS_LOG_ERROR, "biz[%ld] filter init failed, ENOMEM", m_bizId);
		return false;
	}
	m_graph->nb_threads = 0;
	return true;
}

int AvsFilter::filter_add_frame(AVFrame* pFrame)
{
	int ret = 0;
	if (need_reconfig(pFrame)) {
		generate_filter_description();
		if ((ret = configure_filters(m_filterDescr.c_str(), pFrame)) < 0)  {
			AS_LOG(AS_LOG_ERROR, "Error when configure filters");
			avfilter_graph_free(&m_graph);
			return ret;
		}

		m_filterChanged = false;
	}	
	
	if (!m_buffersink_ctx) {
		return AVERROR(EINVAL);
	}
	
	if ((ret = av_buffersrc_add_frame(m_buffersrc_ctx, pFrame)) < 0) {
		AS_LOG(AS_LOG_ERROR,"Error while feeding the filtergraph");
		return ret;
	}
	return ret;
}

int AvsFilter::filter_get_frame(AVFrame* picSink)
{
	return av_buffersink_get_frame_flags(m_buffersink_ctx, picSink, 0);
}

#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)

int AvsFilter::configure_filtergraph(const char* filtergraph, AVFilterContext* source_ctx, AVFilterContext* sink_ctx)
{
	int ret, i;
	int nb_filters = m_graph->nb_filters;
	AVFilterInOut* outputs = NULL, * inputs = NULL;
	char err_buf[256] = { 0 };

	do {
		if (filtergraph) {
			outputs = avfilter_inout_alloc();
			inputs = avfilter_inout_alloc();
			if (!outputs || !inputs) {
				ret = AVERROR(ENOMEM);
				break;
			}

			outputs->name = av_strdup("in");
			outputs->filter_ctx = source_ctx;
			outputs->pad_idx = 0;
			outputs->next = NULL;

			inputs->name = av_strdup("out");
			inputs->filter_ctx = sink_ctx;
			inputs->pad_idx = 0;
			inputs->next = NULL;

			if ((ret = avfilter_graph_parse_ptr(m_graph, filtergraph, &inputs, &outputs, NULL)) < 0) {
				AS_LOG(AS_LOG_ERROR, "avfilter_graph_parse_ptr %s err, %d.", filtergraph, ret);
				break;
			}
		}
		else {
			if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0) {
				AS_LOG(AS_LOG_ERROR, "avfilter_link err, %d.", ret);
				break;
			}
		}

	} while (0);
	
	if (ret < 0) {
		av_strerror(ret, err_buf, sizeof(err_buf) - 1);
		AS_LOG(AS_LOG_ERROR, "biz[%ld] avfilter configure filtergraph err, %s.", m_bizId, err_buf);
		avfilter_inout_free(&outputs);
		avfilter_inout_free(&inputs);
		return ret;
	}

	for (i = 0; i < m_graph->nb_filters - nb_filters; i++) {
		FFSWAP(AVFilterContext*, m_graph->filters[i], m_graph->filters[i + nb_filters]);
	}

	if ((ret = avfilter_graph_config(m_graph, NULL)) < 0) {
		av_strerror(ret, err_buf, sizeof(err_buf) - 1);
		AS_LOG(AS_LOG_ERROR, "avfilter_graph_config err, %s.", err_buf);
		return ret;
	}

	avfilter_inout_free(&outputs);
	avfilter_inout_free(&inputs);
	return ret;
}
