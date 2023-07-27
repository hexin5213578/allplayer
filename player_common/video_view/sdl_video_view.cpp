#include "as.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "sdl_video_view.h"
#include <SDL2/SDL.h>
#include "avs_errno.h"
#include "view_factory.h"
extern "C" {
#include  <libavutil/avutil.h>
}

int SDLVideoView::m_d3d11Index = -1;

int SDLVideoView::getDX11RenderDriver()
{
	SDL_RendererInfo info;
	SDL_GetRendererInfo(nullptr, &info);

	for (int i = 0; i < 5; ++i) {
		SDL_GetRenderDriverInfo(i, &info);
		if (0 == strcmp(info.name, "direct3d11")) {
			m_d3d11Index = i;
			break;
		}
	}
	return m_d3d11Index;
}

int SDLVideoView::init()
{
	if (!m_hWnd) {
		return SDL_ERROR_HWND_INVALID;
	}
	
	std::unique_lock<std::mutex> lck(m_sdlMutex);
	m_sdlWindow = SDL_CreateWindowFrom(m_hWnd);
	if (!m_sdlWindow) {
		AS_LOG(AS_LOG_ERROR, "SDL_CreateWindowFrom failed: %s", SDL_GetError());
		return SDL_ERROR_CREATE_WIN;
	}
	SDL_ShowWindow(m_sdlWindow);
	SDL_GetWindowSize(m_sdlWindow, &m_sdlWidth, &m_sdlHeight);

	if (m_bHwAccel) {
		m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, m_d3d11Index, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (!m_sdlRenderer) {
			AS_LOG(AS_LOG_ERROR, "SDL_CreateRenderer with hwaccel failed: %s, adapt to soft render.", SDL_GetError());
			ViewFactory::getInstance()->setRenderType(SDL_SOFT);
			m_bHwAccel = false;
			m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_SOFTWARE);
		}
	}
	else {
		m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_SOFTWARE);
	}

	if (!m_sdlRenderer) {
		AS_LOG(AS_LOG_ERROR, "SDL_CreateRenderer with hwaccel %d failed: %s", m_bHwAccel, SDL_GetError());
		SDL_DestroyWindow(m_sdlWindow);
		return SDL_ERROR_CREATE_RENDERR;
	}

	SDL_RendererInfo renderer_info = { 0 };
	if (SDL_GetRendererInfo(m_sdlRenderer, &renderer_info)) {
		AS_LOG(AS_LOG_ERROR, "SDL_GetRendererInfo failed: %s", SDL_GetError());
		SDL_DestroyRenderer(m_sdlRenderer);
		SDL_DestroyWindow(m_sdlWindow);
		return SDL_ERROR_GETINFO;
	}
	return SDL_ERROR_SUCCESS;
}

bool SDLVideoView::setTexturePara(int width, int height, Format fmt)
{
	if (reallocTexture(&m_sdlTexture, fmt, width, height) || !m_sdlTexture)
		return false;
	
	m_format = fmt;
	m_width = width;
	m_height = height;
	return true;
}

void SDLVideoView::close()
{
	std::unique_lock<std::mutex> lck(m_sdlMutex);
	if (m_sdlTexture) {
		SDL_DestroyTexture(m_sdlTexture);
		m_sdlTexture = nullptr;
	}
	if (m_sdlRenderer) {
		SDL_DestroyRenderer(m_sdlRenderer);
		m_sdlRenderer = nullptr;
	}
	if (m_sdlWindow) {
		SDL_DestroyWindow(m_sdlWindow);
		m_sdlWindow = nullptr;
	}
	m_cache.clear();
	m_hWnd = nullptr;
}

bool SDLVideoView::snapPicture(std::string savePath)
{
	std::unique_lock<std::mutex> lck(m_sdlMutex);
	if (!m_sdlRenderer || !m_sdlTexture) {
		return false;
	}
	auto target = SDL_GetRenderTarget(m_sdlRenderer);

	SDL_SetRenderTarget(m_sdlRenderer, m_sdlTexture);
	int width, height;
	SDL_QueryTexture(m_sdlTexture,nullptr, nullptr, &width, &height);
	SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);

	SDL_Rect rect{ 0,0,width,height };
	SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, &rect, &rect);

	SDL_RenderReadPixels(m_sdlRenderer, nullptr, surface->format->format, surface->pixels, surface->pitch);
	SDL_SaveBMP(surface, savePath.c_str());
	
	SDL_FreeSurface(surface);
	SDL_SetRenderTarget(m_sdlRenderer, target);
	return false;
}

bool SDLVideoView::draw(const uint8_t* y, int y_pitch, const uint8_t* u, int u_pitch, const uint8_t* v, int v_pitch)
{
	//参数检查
	if (!y || !u || !v) {
		return false;
	}
	std::unique_lock<std::mutex> lck(m_sdlMutex);
	if (!m_sdlTexture || !m_sdlRenderer || !m_sdlWindow || m_width < 0 || m_height < 0) {
		return false;
	}

	if (m_bHwAccel) {
		int w, h;
		SDL_GetWindowSize(m_sdlWindow, &w, &h);
		if (m_sdlWidth != w || m_sdlHeight != h) {
			SDL_DestroyTexture(m_sdlTexture);
			SDL_DestroyRenderer(m_sdlRenderer);
			SDL_DestroyWindow(m_sdlWindow);
			m_sdlTexture = nullptr;
			
			m_sdlWindow = SDL_CreateWindowFrom(m_hWnd);
			if (!m_sdlWindow) {
				return false;
			}
			SDL_ShowWindow(m_sdlWindow);
			m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, m_d3d11Index, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (!m_sdlRenderer) {
				AS_LOG(AS_LOG_ERROR, "SDL_CreateRenderer with hwaccel failed: %s, adapt to soft render.", SDL_GetError());
				return false;
			}
			if (!setTexturePara(m_width, m_height, m_format)) {
				return false;
			}
			m_sdlWidth = w, m_sdlHeight = h;
		}
	}

	int ret = 0;
	if (y_pitch > 0 && u_pitch > 0 && v_pitch > 0) {
		ret = SDL_UpdateYUVTexture(m_sdlTexture, nullptr, y, y_pitch, u, u_pitch, v, v_pitch);
	}
	else if (y_pitch < 0 && u_pitch < 0 && v_pitch < 0) {
		ret = SDL_UpdateYUVTexture(m_sdlTexture, nullptr, y + y_pitch * (m_height -1), -y_pitch, 
							u + u_pitch * (AV_CEIL_RSHIFT(m_height, 1) - 1), -u_pitch,
							v + v_pitch * (AV_CEIL_RSHIFT(m_height, 1) -1 ), -v_pitch);
	}
	else {
		AS_LOG(AS_LOG_WARNING, "Mixed negative and positive linesizes are not supported.");
		ret = -1;
	}

	if (ret < 0) {
		AS_LOG(AS_LOG_WARNING, "SDL_UpdateYUVTexture failed");
		return false;
	}

	SDL_SetRenderDrawColor(m_sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(m_sdlRenderer);

	const SDL_Rect* pRect = nullptr;
	if (-1 != m_iTop && -1 != m_iRight && -1 != m_iBottom && -1 != m_iLeft) {
		SDL_Rect digitRect;
		digitRect.x = m_width * (double)m_iLeft / 100.0;
		digitRect.w = m_width * (double)(m_iRight - m_iLeft) / 100.0;
		digitRect.y = m_height * (double)m_iTop / 100.0;
		digitRect.h = m_height * (double)(m_iBottom - m_iTop) / 100.0;
		pRect = &digitRect;
	}

	if (SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, pRect, nullptr)) {
		//SDL Reset: INVALIDCALL:创建窗口的和渲染放在同一线程
		const char* buf = SDL_GetError();
		AS_LOG(AS_LOG_WARNING, "SDL_RenderCopy failed %s", buf);
		return false;
	}
	SDL_RenderPresent(m_sdlRenderer);
	return true;
}

bool SDLVideoView::draw(uint8_t* data, int linesize)
{
	if (!data) return false;
	std::unique_lock<std::mutex> lck(m_sdlMutex);
	if (!m_sdlTexture || !m_sdlRenderer || !m_sdlWindow || m_width < 0 || m_height < 0) 
		return false;

	if (m_bHwAccel) {
		int w, h;
		SDL_GetWindowSize(m_sdlWindow, &w, &h);
		if (m_sdlWidth != w || m_sdlHeight != h) {
			/*SDL_DestroyTexture(m_sdlTexture);
			SDL_DestroyRenderer(m_sdlRenderer);
			SDL_DestroyWindow(m_sdlWindow);
			m_sdlTexture = nullptr;

			m_sdlWindow = SDL_CreateWindowFrom(m_hWnd);
			if (!m_sdlWindow) {
				return false;
			}
			SDL_ShowWindow(m_sdlWindow);
			m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, m_d3d11Index, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (!m_sdlRenderer) {
				AS_LOG(AS_LOG_ERROR, "SDL_CreateRenderer with hwaccel failed: %s, adapt to soft render.", SDL_GetError());
				return false;
			}
			if (!setTexturePara(m_width, m_height, m_format)) {
				return false;
			}*/
			m_sdlWidth = w, m_sdlHeight = h;
		}
	}

	int ret = 0;
	if (linesize < 0) {
		ret = SDL_UpdateTexture(m_sdlTexture, nullptr, data + linesize * (m_height - 1), -linesize);
	}
	else {
		ret = SDL_UpdateTexture(m_sdlTexture, nullptr, data, linesize);
	}

	if (ret < 0) {
		AS_LOG(AS_LOG_WARNING, "SDL_UpdateTexture failed");
		return false;
	}

	SDL_SetRenderDrawColor(m_sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(m_sdlRenderer);

	const SDL_Rect* pRect = nullptr;
	if (-1 != m_iTop && -1 != m_iRight && -1 != m_iBottom && -1 != m_iLeft) {
		SDL_Rect digitRect;
		digitRect.x = m_width * (double)m_iLeft / 100.0;
		digitRect.w = m_width * (double)(m_iRight - m_iLeft) / 100.0;
		digitRect.y = m_height * (double)m_iTop / 100.0;
		digitRect.h = m_height * (double)(m_iBottom - m_iTop) / 100.0;
		pRect = &digitRect;
	}

	if (SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, pRect, nullptr)) {
		//SDL Reset: INVALIDCALL:创建窗口的和渲染放在同一线程
		const char* buf = SDL_GetError();
		AS_LOG(AS_LOG_WARNING, "SDL_RenderCopy failed %s", buf);
		return false;
	}
	SDL_RenderPresent(m_sdlRenderer);
	return true;
}

int SDLVideoView::reallocTexture(SDL_Texture** texture, Format fmt, int new_width, int new_height)
{
	int access, w, h;
	uint32_t oldFmt;
	uint32_t newFmt = SDL_PIXELFORMAT_IYUV;
	switch (fmt) {
	case AvsVideoView::YUV420p:
		newFmt = SDL_PIXELFORMAT_IYUV;
		break;
	case AvsVideoView::NV12:
		newFmt = SDL_PIXELFORMAT_NV12;
		break;
	default:
		break;
	}

	if (!*texture || SDL_QueryTexture(*texture, &oldFmt, &access, &w, &h) < 0
		|| new_width != w || new_height != h || newFmt != oldFmt) 
	{
		if (*texture) {
			SDL_DestroyTexture(*texture);
		}
		
		if (SDL_PIXELFORMAT_NV12 == newFmt) {
			if (new_width != m_width || new_height != m_height) {
				m_cache.resize(new_width * new_height * 2);
				m_cache.shrink_to_fit();
			}
		}

		if (!(*texture = SDL_CreateTexture(m_sdlRenderer, newFmt, SDL_TEXTUREACCESS_STREAMING, new_width, new_height))) {
			const char* buf = SDL_GetError();
			AS_LOG(AS_LOG_WARNING, "SDL_CreateTexture failed %s", buf);
			return -1;
		}
		AS_LOG(AS_LOG_INFO, "Created %dx%d texture with %s.", new_width, new_height, SDL_GetPixelFormatName(newFmt));
	}
	return 0;
}

void SDLVideoView::clear()
{
	std::unique_lock<std::mutex> lck(m_sdlMutex);
	if (m_sdlWindow) {
		SDL_HideWindow(m_sdlWindow);
		SDL_ShowWindow(m_sdlWindow);
	}
}

#endif 