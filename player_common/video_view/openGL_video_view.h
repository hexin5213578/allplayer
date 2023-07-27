//
// Created by HP1 on 2021/7/26.
//

#ifndef PRJ_ANDROID_OPENGL_VIDEO_VIEW_H
#define PRJ_ANDROID_OPENGL_VIDEO_VIEW_H
//TODO 重新编写渲染

#include "avs_video_view.h"
#include <mutex>
#include <string>

class XTexture;

class OpenGLVideoView : public AvsVideoView
{
public:
    OpenGLVideoView(void * hwnd, AvsVideoView::ViewType type): AvsVideoView(hwnd, type) {}
    int init();

    bool setTexturePara(int width, int height, Format fmt) override;

    void close() override;

    void clear() override;

    bool draw(const unsigned  char* y, int y_pitch,
          const unsigned  char* u, int u_pitch,
          const unsigned  char* v, int v_pitch) override;

    virtual bool draw(unsigned char *data[],int width,int height, int surfaceWidth, int surfaceHeight) override;

    void clearSurface() override;

    virtual bool snapPicture(std::string savePath) override;

private:
    int reallocTexture(XTexture** tex, Format fmt, int new_width, int new_height);

private:
    std::mutex		m_sdlMutex;
    uint32_t        m_fmt = -1;
    XTexture*       m_texture = nullptr;
};

#endif //PRJ_ANDROID_OPENGL_VIDEO_VIEW_H
