#pragma once

#include "avs_video_view.h"
#include <mutex>

class SDL_Window;
class SDL_Texture;
class SDL_Renderer;

class SDLVideoView : public AvsVideoView
{
public:
	SDLVideoView(void* hwnd, ViewType type, bool hwaccel = false)
		:AvsVideoView(hwnd, type),
		m_sdlTexture(nullptr), m_sdlWindow(nullptr), m_sdlRenderer(nullptr),
		m_bHwAccel(hwaccel),
		m_sdlWidth(0), m_sdlHeight(0) {
	}
	virtual ~SDLVideoView() = default;

	static int getDX11RenderDriver();

protected:
	int init() override;

	bool setTexturePara(int width, int height, Format fmt) override;

	void close() override;

	void clear() override;

	bool snapPicture(std::string savePath) override;

	bool draw(const uint8_t* y, int y_pitch,
			  const uint8_t* u, int u_pitch,
			  const uint8_t* v, int v_pitch) override;

	bool draw(uint8_t* data, int linesize) override;

private:
	int reallocTexture(SDL_Texture** tex, Format fmt, int new_width, int new_height);

private:
	static	int		m_d3d11Index;
	std::mutex		m_sdlMutex;
	SDL_Texture*	m_sdlTexture;
	SDL_Window*		m_sdlWindow;
	SDL_Renderer*	m_sdlRenderer;
	bool			m_bHwAccel;
	int				m_sdlWidth, m_sdlHeight;
};

