#include "avs_watermark.h"
#include "as.h"

const char* kPadding = "20";

int WaterMarkManager::parse_configure(const std::string& configure)
{
	AS_LOG(AS_LOG_INFO, "start parse watermark configure %s.", configure.data());
	cJSON* config = cJSON_Parse(configure.c_str());
	if (!config) {
		return -1;
	}
	cJSON* text = cJSON_GetObjectItem(config, "Text");
	if (!text || !text->valuestring) {
		release();
		AS_LOG(AS_LOG_WARNING, " invalid text json object.");
		return AS_ERROR_CODE_INVALID;
	}
	else if ('\0' == text->valuestring[0]) {  
		release();
		AS_LOG(AS_LOG_INFO, "empty text, cancel watermark.");
		return 0;
	}

	cJSON* fontFile = cJSON_GetObjectItem(config, "FontFile");
	if (fontFile && fontFile->valuestring) {
		m_fontFile = std::string(fontFile->valuestring);
	}
	else {
		m_fontFile = "allplayer.ttf";
	}
	
	cJSON* fontColor = cJSON_GetObjectItem(config, "FontColor");
	cJSON* fontSize = cJSON_GetObjectItem(config, "FontSize");
	cJSON* alpha = cJSON_GetObjectItem(config, "Alpha");
	cJSON* position = cJSON_GetObjectItem(config, "Position");
	
	cJSON* localTime = cJSON_GetObjectItem(config, "LocalTime");
	cJSON* renderOn = cJSON_GetObjectItem(config, "RenderOn");
	cJSON* writeOn = cJSON_GetObjectItem(config, "WriteOn");
	cJSON* layout = cJSON_GetObjectItem(config, "Layout");

	if (!(m_wtMarkFilter = AS_NEW(m_wtMarkFilter, 512))) {
		return AS_ERROR_CODE_MEM;
	}

	if (!(m_text = AS_NEW(m_text, 256))) {
		release();
		return AS_ERROR_CODE_MEM;
	}
	memcpy(m_text, text->valuestring, 256);

	m_fontColor = fontColor ? std::string(fontColor->valuestring) : "";
	m_fontSize = fontSize ? fontSize->valueint : 14;
	m_alpha = alpha ? alpha->valuedouble : 0.5;
	m_position = position ? (WtMarkPos)position->valueint : kTopLeft;
	if (localTime) {
		m_localTime = localTime->valueint;
	}
	if (m_localTime) {
		if (layout) m_layout = (WtLayout)layout->valueint;
		snprintf(m_text, 256, "%s%c%c{localtime%s}", m_text, kTopBottom == m_layout ? '\n' : ' ', '%', "\\:%Y-%m-%d");
	}

	if (m_fontColor.empty()) {
		snprintf(m_wtMarkFilter, 512, "drawtext=text=\'%s\':fontfile=%s:fontsize=%d:alpha=%lf:line_spacing=16",
			m_text, m_fontFile.c_str(), m_fontSize, m_alpha);
	}
	else {
		snprintf(m_wtMarkFilter, 512, "drawtext=text=\'%s\':fontfile=%s:fontsize=%d:fontcolor=%s:alpha=%lf:line_spacing=16",
			m_text, m_fontFile.c_str(), m_fontSize, m_fontColor.c_str(), m_alpha);
	}

	std::string xtext = kPadding, ytext = kPadding;

	switch (m_position) {
	case kTopLeft:		// top left
		//kPadding
		break;
	case kTopRight:
		xtext = "w-tw" + std::string("-") + std::string(kPadding), ytext = kPadding;
		break;
	case kBottomLeft:
		xtext = kPadding;
		ytext = "h" + std::string("-th") + std::string("-") + std::string(kPadding);
		break;
	case kBottomRight:
		xtext = "w-tw" + std::string("-") + std::string(kPadding);
		ytext = "h" + std::string("-th") + std::string("-") + std::string(kPadding);
		break;
	default:
		break;
	}

	snprintf(m_wtMarkFilter, 512, "%s:x=%s:y=%s", m_wtMarkFilter, xtext.c_str(), ytext.c_str());
	if (renderOn) {
		m_renderOn = renderOn->valueint;
	}
	if (writeOn) {
		m_writeOn = writeOn->valueint;
	}
	m_waterOn = 1;
	return 0;
}

ACWaterMark* WaterMarkManager::produce_watermark(long id)
{
	if (!m_waterOn) {
		return nullptr;
	}

	ACWaterMark* waterMark = AS_NEW(waterMark);
	if (!waterMark) {
		return nullptr;
	}

	if (!waterMark->init(id)) {
		AS_DELETE(waterMark);
		return nullptr;
	}

	std::string szWaterMark(m_wtMarkFilter);
	waterMark->parse_filter_params(szWaterMark);
	return waterMark;
}

WaterMarkManager::~WaterMarkManager()
{
	release();
}

void WaterMarkManager::release()
{
	m_waterOn = false;
	AS_DELETE(m_wtMarkFilter);
	AS_DELETE(m_text);
}

ACWaterMark::ACWaterMark()
	:AvsFilter(),
	m_lastWidth(0),
	m_lastHeight(0)
{
}

bool ACWaterMark::need_reconfig(AVFrame* frame)
{
	return m_graph &&
		(m_filterChanged || (m_lastWidth != frame->width) || (m_lastHeight != frame->height) || (m_lastFormat != frame->format));
}

int ACWaterMark::configure_filters(const char* filters, AVFrame* frame)
{
	int ret = -1;
	if (need_reconfig(frame)) {
		AS_LOG(AS_LOG_INFO, "biz[%ld] frame change from size:%dx%d format:%s to size:%dx%d format:%s",
			m_bizId,
			m_lastWidth, m_lastHeight,
			(const char*)av_x_if_null(av_get_pix_fmt_name((AVPixelFormat)m_lastFormat), "none"),
			frame->width, frame->height,
			(const char*)av_x_if_null(av_get_pix_fmt_name((AVPixelFormat)frame->format), "none"));

		char buffersrc_args[128];
		AVFilterContext* filt_src = NULL, * filt_out = NULL, * last_filter = NULL;

		snprintf(buffersrc_args, sizeof(buffersrc_args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d",
			frame->width, frame->height, frame->format,
			1, 1000);

		char err_buf[128] = { 0 };
		do {
			if ((ret = avfilter_graph_create_filter(&filt_src, avfilter_get_by_name("buffer"), "watermark_buffer", buffersrc_args, NULL, m_graph)) < 0) {
				AS_LOG(AS_LOG_WARNING, "avfilter_graph_create_filter(buffer) err: %d.", ret);
				break;
			}

			ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink"), "watermark_buffersink", NULL, NULL, m_graph);
			if (ret < 0) {
				AS_LOG(AS_LOG_WARNING, "avfilter_graph_create_filter(buffersink) err: %d.", ret);
				break;
			}


			if ((ret = configure_filtergraph(filters, filt_src, filt_out)) < 0) {
				AS_LOG(AS_LOG_WARNING, "configure_filtergraph %s err: %d.", filters, ret);
				break;
			}

		} while (0);

		if (ret < 0) {
			av_strerror(ret, err_buf, sizeof(err_buf) - 1);
			AS_LOG(AS_LOG_WARNING, "biz[%ld] configure watermark filters err: %s.", m_bizId, err_buf);
			return ret;
		}

		m_buffersrc_ctx = filt_src;
		m_buffersink_ctx = filt_out;
		m_lastWidth = frame->width;
		m_lastHeight = frame->height;
		m_lastFormat = (AVPixelFormat)frame->format;
		return ret;
	}
	return ret;
}

ACWaterMark::~ACWaterMark()
{
}

int ACWaterMark::parse_filter_params(std::string& params)
{
	m_filterDescr = params;
	m_filterChanged = true;
	return 0;
}
