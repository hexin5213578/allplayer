#pragma once

#include "avs_filter.h"

enum WtMarkPos
{
	kTopLeft = 0,
	kTopRight = 1,
	kBottomLeft = 2,
	kBottomRight = 3,
};

enum WtLayout
{
	kTopBottom = 0,
	kLeftRight = 1,
};

class ACWaterMark;

class WaterMarkManager
{
public:
	static WaterMarkManager* get_instance() {
		static WaterMarkManager manager;
		return &manager;
	}

	//当前状态是否需要水印
	bool get_watermark_on(bool isRendering = false) {
		if (!m_waterOn) {
			return false;
		}
		return isRendering ? m_renderOn : m_writeOn;
	}

	int	parse_configure(const std::string& configure);

	ACWaterMark* produce_watermark(long id);

	virtual ~WaterMarkManager();

private:
	WaterMarkManager()
		: m_waterOn(false)
		, m_wtMarkFilter(nullptr)
		, m_layout(kTopBottom)
		, m_localTime(0)
		, m_renderOn(false)
		, m_writeOn(false) {
	}

	void release();

	bool			m_waterOn;
	char*			m_wtMarkFilter;
	char*			m_text;
	std::string		m_fontFile;
	std::string		m_fontColor;
	int				m_fontSize;
	double			m_alpha;
	WtMarkPos		m_position;
	WtLayout		m_layout;
	uint8_t			m_localTime;
	bool			m_renderOn;
	bool			m_writeOn;
};

class ACWaterMark : public AvsFilter
{
public:
	ACWaterMark();

	virtual ~ACWaterMark();

	int parse_filter_params(std::string& params) override;

protected:
	bool need_reconfig(AVFrame* frame) override;

	int configure_filters(const char* filters, AVFrame* frame) override;

private:
	int		m_lastWidth;
	int		m_lastHeight;
};
