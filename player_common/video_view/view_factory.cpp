#include "view_factory.h"
#include "as_config.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "sdl_video_view.h"
#include <SDL2/SDL.h>
#include "d3d11_video_view.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "openGL_video_view.h"
#endif

ViewFactory* ViewFactory::getInstance() {
	static ViewFactory viewFactory;
	return &viewFactory;
}
             
void ViewFactory::setRenderType(RENDER_TYPE type) 
{
	m_renderType = type;

#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
	if (SDL_SOFT == m_renderType || SDL_ACCELERATED == m_renderType) {
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
	}

	if (SDL_SOFT == m_renderType) {
		UINT8 state = SDL_GetEventState(SDL_WINDOWEVENT);
		if (SDL_IGNORE == state) {
			SDL_EventState(SDL_WINDOWEVENT, SDL_ENABLE);  
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
			SDL_EventState(SDL_USEREVENT, SDL_ENABLE);
		}
	}
	else if (SDL_ACCELERATED == m_renderType) {
		UINT8 state = SDL_GetEventState(SDL_WINDOWEVENT);
		if (SDL_ENABLE == state) {
			//忽略windows窗户事件，窗口resize不会影响sdl
			SDL_EventState(SDL_WINDOWEVENT, SDL_IGNORE);
			SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
			SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
		}
	}
#endif
}

AvsVideoView* ViewFactory::createVideoView(void* window)
{
	if(m_videoViews.count(window) > 0 && !m_videoViews[window]) {
		m_videoViews.erase(window);
	}

	if (0 == m_videoViews.count(window)) {
		AvsVideoView* view = nullptr;
		switch (m_renderType) {
		case SDL_SOFT:
		case SDL_ACCELERATED:
#ifdef _WIN32
			view = new SDLVideoView(window, AvsVideoView::kSDL, SDL_ACCELERATED == m_renderType);
			if (view->init()) {
				AS_LOG(AS_LOG_ERROR, "view for window:[%p] init failed!", window);
				view->close();
				AS_DELETE(view);
			}
#endif // _WIN32
			break;
		case OPENGL:
			#if ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
			view = new OpenGLVideoView(window, AvsVideoView::kNONE);
			#endif
			break;
		default:
			break;
		}

		if (!view) {
			AS_LOG(AS_LOG_ERROR, "view for window:[%p] malloc failed, out of memory!", window);
			return view;
		}

		m_videoViews.emplace(window, view);
	}
	return m_videoViews[window];
}

AvsVideoView* ViewFactory::createD3DVideoView(void* window)
{
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
	if (0 == m_videoViews.count(window)) {
		AvsVideoView* view = new D3D11VideoView(window, AvsVideoView::kD3D11);
		if (!view) {
			return nullptr;
		}

		int32_t ret = view->init();
		if (ret) {
			AS_LOG(AS_LOG_ERROR, "d3d view for window:[%p] init failed, %d.!", window, ret);
			view->close();
			delete view;
			return nullptr;
		}
		m_videoViews.insert(std::make_pair(window, view));
	}
	return m_videoViews[window];
#endif
    return nullptr;
}

bool ViewFactory::detroyViewView(void* window)
{
	if (0 == m_videoViews.count(window)) {
		return false;
	}
	else {
		auto view = m_videoViews[window];
		view->close();
		delete view;
		m_videoViews.erase(window);
	}
	return true;
}

void ViewFactory::releaseVideoViews()
{
	for (auto iter : m_videoViews) {
		auto view = iter.second;
		view->close();
		delete view;
	}
	m_videoViews.clear();
}

ViewFactory::ViewFactory()
{
//#ifdef _WIN64
//	if (SDLVideoView::getDX11RenderDriver() >= 0) {
//		m_renderType = SDL_ACCELERATED;
//	}
//	else {
//		m_renderType = SDL_SOFT;
//	}
//#else 
//	m_renderType = SDL_SOFT;
//#endif
	m_renderType = SDL_SOFT;
	setRenderType(m_renderType);
}