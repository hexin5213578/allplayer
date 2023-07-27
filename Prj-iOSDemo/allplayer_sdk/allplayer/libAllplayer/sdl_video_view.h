#pragma once
#include "ac_video_view.h"

class SDL_Window;
class SDL_Texture;
class SDL_Renderer;

class SDLVideoView : public ACVideoView
{
	bool init(void* hwnd) override;

	bool setTexturePara(int width, int height, Format fmt) override;

	void close() override;

	bool snapPicture(std::string savePath) override;

	bool draw(const unsigned  char* y, int y_pitch,
			  const unsigned  char* u, int u_pitch,
			  const unsigned  char* v, int v_pitch) override;

private:
	SDL_Texture*	m_sdlTexture = nullptr;
	SDL_Window*		m_sdlWindow = nullptr;
	SDL_Renderer*	m_sdlRenderer = nullptr;
};

