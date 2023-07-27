#pragma once

#include "view_factory.h"
#include "avs_thread_base.h"
#include "avs_video_state.h"

class AvsRefreshLoop : public AvsThreadBase
{
public:
	AvsRefreshLoop(VideoState* ic): m_vidState(ic)	{
		if (!m_vidState) {
			throw std::invalid_argument("invalid ic for refresh loop.");
		}
	}
	virtual ~AvsRefreshLoop() = default;

	void setDuration(int64_t duration) { m_duration = duration;	}
	int setView(AvsVideoView::Ptr view);

	void surfaceDiff(int width, int height) {
		m_surfaceWidth = width;
		m_surfaceHeight = height;
	}

	int getFps() { return m_fps; }
	int zoomOut(void* hwnd, int top, int right, int bottom, int left);
	int zoonIn();

	void getStallingInfo(int& stallEvent, double& stallaDuration) {
		stallEvent = m_stallEventNum;
		stallaDuration = m_stallDuration;
		m_stallEventNum = 0;
		m_stallDuration = 0.0;
	}

	void stopRun() override;

	void setWindow(void* win) { 
		if (m_hwnd != win) {
			m_hwnd = win; 
			m_surfaceDiff = true;
		}
	}
protected:
	void mainProcess() override {
		videoRefresh();
	}

	virtual int implDraw(AvsVideoView* view, AVFrame* frame);
    
    void*                m_hwnd = nullptr;
    
    int                    m_surfaceWidth = 0;
    int                    m_surfaceHeight = 0;

private:
	void videoRefresh();
	void render(MyFrame::Ptr frame);

private:
	VideoState*			m_vidState = nullptr;
	AvsVideoView::Ptr	m_view;
	std::mutex			m_zoomMutex;
	bool				m_zooming = false;
	AvsVideoView*		m_zoomView = nullptr;
	int64_t				m_duration = 0;
	bool				m_rendered = false;
	
	bool				m_surfaceDiff = false;
	int					m_fps = 0;
	double				m_lastOutputTS = -1.10;

	int					m_stallEventNum = 0;
	double				m_stallDuration = 0.0;
};
