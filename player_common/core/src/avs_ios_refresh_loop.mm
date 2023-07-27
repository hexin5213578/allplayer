#include "avs_ios_refresh_loop.h"
#import "ios_openGL_view.h"

int AvsIosRefreshLoop::implDraw(AvsVideoView* view, AVFrame* frame)
{
	if (!frame) {
        return AVERROR(ENOMEM);

	}

	if (!m_glView) {
        m_glView = new IOSOpenGLVideoView(m_hwnd, AvsVideoView::ViewType::kNONE);
//		m_glView->init();
	}

	if (!m_glView->setTexturePara(frame->width, frame->height, (AvsVideoView::Format)frame->format)) {
        return AVERROR(EINVAL);

	}

    m_glView->drawFrame(frame, m_surfaceWidth, m_surfaceHeight);
    
    return 0;
}

void AvsIosRefreshLoop::stopRun()
{
	AvsRefreshLoop::stopRun();
   if(m_glView) {
        m_glView->close();
        delete m_glView;
    }
}
