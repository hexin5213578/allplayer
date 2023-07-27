#pragma once
#include "avs_refresh_loop.h"
#include "ios_openGL_view.h"

class IOSOpenGLVideoView;

class AvsIosRefreshLoop : public AvsRefreshLoop
{
public:
    AvsIosRefreshLoop(VideoState *is) : AvsRefreshLoop(is) {};
protected:
	int implDraw(AvsVideoView* view, AVFrame* frame);
	void stopRun();
private:
	IOSOpenGLVideoView*     m_glView = nullptr;
};

