#include "sdl_video_view.h"
//#include <SDL2/SDL.h>
#include "SDL.h"
bool SDLVideoView::init(void* hwnd)
{
	if (!hwnd) {
		return false;
	}
	m_hWnd = hwnd;
	//忽略windows窗户事件，窗口resize不会影响sdl
	SDL_EventState(SDL_WINDOWEVENT, SDL_IGNORE);
	return true;
}

bool SDLVideoView::setTexturePara(int width, int height, Format fmt)
{
	//说明已经初始化过
	if (m_sdlTexture) {
		return true;
	}

	m_sdlWindow = SDL_CreateWindowFrom(m_hWnd);
	SDL_ShowWindow(m_sdlWindow);

	if (!m_sdlWindow) {
		return false;
	}
	m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_ACCELERATED);
	if (!m_sdlRenderer) {
		return false;
	}
	unsigned int sdl_fmt = SDL_PIXELFORMAT_IYUV;
	switch (fmt) {
	case ACVideoView::YUV420p:
		sdl_fmt = SDL_PIXELFORMAT_IYUV;
		break;
	case ACVideoView::NV12:
		sdl_fmt = SDL_PIXELFORMAT_NV12;
		break;
	default:
		break;
	}
	m_sdlTexture = SDL_CreateTexture(m_sdlRenderer, sdl_fmt, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!m_sdlTexture) {
		return false;
	}
	return true;
}

void SDLVideoView::close()
{
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
}

bool SDLVideoView::snapPicture(std::string savePath)
{
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

bool SDLVideoView::draw(const unsigned char* y, int y_pitch, const unsigned char* u, int u_pitch, const unsigned char* v, int v_pitch)
{
	//参数检查
	if (!y || !u || !v) {
		return false;
	}

	if (!m_sdlTexture || !m_sdlRenderer || !m_sdlWindow || m_width < 0 || m_height < 0) {
		return false;
	}
	
	if (SDL_UpdateYUVTexture(m_sdlTexture, nullptr, y, y_pitch, u, u_pitch, v, v_pitch)) {
		return false;
	}

	SDL_RenderClear(m_sdlRenderer);
	if (SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, nullptr, nullptr)) {
		//SDL Reset: INVALIDCALL:创建窗口的和渲染放在同一线程
		const char* buf = SDL_GetError();
		return false;
	}

	SDL_RenderPresent(m_sdlRenderer);
	return true;
}
