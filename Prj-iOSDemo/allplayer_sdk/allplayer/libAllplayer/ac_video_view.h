/*****************************************************************//**
 * \file   ac_video_view.h
 * \brief  视频渲染接口类
 * 
 * \author songgan
 * \date   April 2021
 *********************************************************************/
#pragma once

#include <memory>
#include <mutex>

struct AVFrame;

class ACVideoView
{
public:
	//枚举值和ffmpeg保持一致
	enum Format {
		YUV420p = 0,
		NV12 = 23,
	};

	enum RenderType {
		SDL = 0,
	};

	static ACVideoView* create(RenderType type = SDL);

	/**
	 * .
	 * 初始化渲染窗口，线程安全
	 * \param hwnd 渲染窗口句柄
	 * \return  
	 */
	virtual bool init(void* hwnd) = 0;

	/**
	 * .
	 * 设置渲染材质参数
	 * \param width
	 * \param height
	 * \param fmt
	 * \return 
	 */
	virtual bool setTexturePara(int width, int height, Format fmt = YUV420p) = 0;

	virtual void close() = 0;

	/**
	 * .
	 * 根据fmt渲染一帧数据
	 * \param frame
	 * \return 
	 */
	bool drawFrame(AVFrame* frame);

	/**
	 * .
	 * 保存正在渲染的图像数据
	 * \param savePath
	 * \return 
	 */
	virtual bool snapPicture(std::string savePath) = 0;

	/**
	 * .
	 * 渲染图像,抽象
	 * \return 
	 */
	virtual bool draw(const unsigned  char* y, int y_pitch,
					  const unsigned  char* u, int u_pitch,
					  const unsigned  char* v, int v_pitch) = 0;

protected:
	ACVideoView();

protected:
	std::mutex m_mtx;
	void* m_hWnd = nullptr;
	//材质宽高
	int m_width = 0;
	int m_height = 0;
	Format m_format = YUV420p;
};

