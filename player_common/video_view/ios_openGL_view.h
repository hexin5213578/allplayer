//
//  ios_openGL_view.hpp
//  playerSDK
//
//  Created by sg on 2021/9/10.
//

#ifndef ios_egl_hpp
#define ios_egl_hpp

#include "as_config.h"
#if((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include "avs_video_view.h"
#include "ac_texture.h"
#include <mutex>

class IOSOpenGLVideoView: public AvsVideoView
{
public:
    IOSOpenGLVideoView(void* hwnd, ViewType type):AvsVideoView(hwnd, type){
    }
    
    virtual ~IOSOpenGLVideoView() {
    }
    
    int init() override;

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
    std::mutex        m_glMutex;
    uint32_t        m_fmt = -1;
    XTexture*       m_texture = nullptr;
};
#endif

#endif /* ios_egl_hpp */
