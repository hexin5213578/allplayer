/*****************************************************************//**
 * \file   ac_video_view.h
 * \brief  视频渲染接口类
 * 
 * \author songgan
 * \date   April 2021
 *********************************************************************/
#pragma once

#include <memory>
#include <string>
#include <mutex>
#include <vector>

struct AVFrame;

class AvsVideoView
{
public:
	//枚举值和ffmpeg保持一致
	enum Format {
		YUV420p = 0,
		YUVJ420P = 12,
		NV12 = 23,
		D3D11 = 174,
	};

	enum ViewType {
		kSDL = 0,
		kD3D11 = 1,
		kNONE = 2
	};

	using Ptr = std::shared_ptr<AvsVideoView>;

    virtual ~AvsVideoView();

	ViewType getViewType() { return m_type; }
    
	/**
	 * 初始化渲染窗口，线程安全
	 * \param hwnd 渲染窗口句柄
	 */
	virtual int init() = 0;

	void* getHwnd() { return m_hWnd; }

	/**
	 * 设置渲染材质参数
	 * \param width
	 * \param height
	 * \param fmt 
	 */
	virtual bool setTexturePara(int width, int height, Format fmt = YUV420p) = 0;

	virtual void close() = 0;

	/**
	 * 根据fmt渲染一帧数据
	 * \param frame
	 * \return 
	 */
	bool drawFrame(AVFrame* frame);
	bool drawFrame(AVFrame* frame,int width, int height);

	/**
	 * 保存正在渲染的图像数据
	 * \param savePath
	 */
	virtual bool snapPicture(std::string savePath) { return true; }

	/**
	 * 渲染图像,抽象
	 */
	virtual bool draw(const uint8_t* y, int y_pitch,
		const uint8_t* u, int u_pitch,
		const uint8_t* v, int v_pitch) {
		return false;
	}

	virtual bool draw(uint8_t* data, int linesize) { return false; }
	virtual bool draw(uint8_t* data[], int width, int height, int surfaceWidth = 0, int surfaceHeight = 0) {  return false; }

	virtual void clearSurface() { }

	/**
	*清理最后一帧画面
	*/
	virtual void clear() = 0;

	/**
	*设置边距比例
	*/
	virtual void setMarginBlankRatio(int iTop = 0, int iRight = 0, int iBottom = 0, int iLeft = 0);

protected:
	AvsVideoView(void* hwnd, ViewType type);

protected:
	uint8_t*		m_datas[3];
	bool 			m_bAllocated = false;
	std::mutex		m_dataMtx;
	int 			m_surfaceWidth = 0;
	int 			m_surfaceHeight = 0;
	void*			m_hWnd;
	int				m_width = 0;	//材质宽高
	int				m_height = 0;
	Format			m_format = YUV420p;
	int				m_iLeft = -1, m_iRight = -1, m_iTop = -1, m_iBottom = -1;	//电子放大的区域(百分比)
	std::vector<uint8_t>	m_cache;
	ViewType		m_type;
};

