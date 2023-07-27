//
// Created by HP1 on 2021/7/26.
//

#include "openGL_video_view.h"
#include "ac_texture.h"
#include "avs_errno.h"
//TODO 重新编写渲染

int OpenGLVideoView::init() {
//    if (m_hWnd == hwnd){
//        return ERR_AGAIN;
//    }
//    if (!hwnd) {
//        return ERR_INVALID;
//    }
//    m_hWnd = hwnd;
    return ERR_SUCCESS;
}

bool OpenGLVideoView::draw(const unsigned char *y, int y_pitch, const unsigned char *u, int u_pitch,
                           const unsigned char *v, int v_pitch) {
//    unsigned char *buf[3];
//    buf[0] = (unsigned char *)y;
//    buf[1] = (unsigned char *)u;
//    buf[2] = (unsigned char *)v;
//    m_texture->Draw(buf,m_width,m_height);
    return false;
}

bool OpenGLVideoView::setTexturePara(int width, int height, AvsVideoView::Format fmt) {
    if(!m_hWnd) return false;

    reallocTexture(&m_texture,fmt,width,height);
    if (!m_texture) {
        return false;
    }
    return true;
}

void OpenGLVideoView::close() {
    if(!m_hWnd) return;
    if(m_texture) {
        m_texture->Drop();
        delete  m_texture;
        m_texture = nullptr;
    }
}

void OpenGLVideoView::clear() {

}

bool OpenGLVideoView::snapPicture(std::string savePath) {
    return false;
}

int OpenGLVideoView::reallocTexture(XTexture **texture, AvsVideoView::Format fmt, int new_width,
                                    int new_height) {
    int access, w, h;
    uint32_t newFmt = XTEXTURE_YUV420P;
    switch (fmt) {
        case AvsVideoView::YUV420p:
        case AvsVideoView::YUVJ420P:
            newFmt = XTEXTURE_YUV420P;
            break;
        case AvsVideoView::NV12:
            newFmt = XTEXTURE_NV12;
            break;
        default:
            break;
    }

    if (!*texture || new_width != m_width || new_height != m_height || newFmt != m_fmt) {
        if (*texture) {
            (*texture)->Drop();
            delete *texture;
        }
        *texture = XTexture::Create();
        if(YUVJ420P ==newFmt) newFmt = YUV420p;
        m_texture->Init(m_hWnd,(XTextureType)newFmt);
        m_fmt = newFmt;
        m_width = new_width;
        m_height = new_height;
    }
    return 0;
}

bool OpenGLVideoView::draw(unsigned char *data[],int width,int height, int surfaceWidth, int surfaceHeight) {
    if(m_texture) {
        if(surfaceHeight || surfaceWidth) {
            m_texture->surfaceChange(surfaceWidth, surfaceHeight);
            m_surfaceWidth = surfaceWidth;
            m_surfaceHeight = surfaceHeight;
        }
        m_texture->Draw(data,width,height);
    }
    return true;
}

void OpenGLVideoView::clearSurface() {
    if(m_texture) {
        m_texture->ClearSurface();
    }
}

