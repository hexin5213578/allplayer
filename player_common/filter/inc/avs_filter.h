#pragma once

#include "as_json.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

extern "C" {
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/pixdesc.h>
}

class AvsFilter
{
public:
	AvsFilter() = default;
	virtual ~AvsFilter();

	bool	init(long bizId);
	
	virtual int parse_filter_params(std::string& params) = 0;

	virtual int filter_add_frame(AVFrame* frame);
	
	/**
	 * 获取filter图像，循环读取
	 * \param picSink
	 * \return >=0:success,otherwise failed.
	 */
	int		filter_get_frame(AVFrame* picSink);

protected:
	virtual bool need_reconfig(AVFrame* frame) = 0;

	virtual int configure_filters(const char* filters, AVFrame* frame) = 0;
	
	//return >=0:success, otherwise failed.
	virtual int	 configure_filtergraph(const char* filtergraph, AVFilterContext* source_ctx, AVFilterContext* sink_ctx);
	
	virtual int	generate_filter_description() { return 0; }

protected:
	std::string			m_filterDescr;
	long				m_bizId = -1;
	int					m_lastFormat = -1;
	bool				m_filterChanged = false;
	AVFilterGraph*		m_graph = nullptr;
	AVFilterContext*	m_buffersrc_ctx = nullptr;
	AVFilterContext*	m_buffersink_ctx = nullptr;
};

