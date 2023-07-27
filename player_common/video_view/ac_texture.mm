//
//  ac_texture.m
//  playerSDK
//
//  Created by sg on 2021/9/10.
//

#include "ac_texture.h"
#include "ac_EGL.h"
#include "ac_shader.h"
#include "app_log.h"
#include "openGL20.h"

class CATexture:public XTexture
{
public:
    ac_shader sh;
    XTextureType type;
    std::mutex mux;
    OpenGLView20* m_pView = nullptr;

    virtual void Drop()
    {
        std::unique_lock<std::mutex> lck(mux);
        if(m_pView) {
            [m_pView Close];
            m_pView = nil;
        }
        sh.Close();
    }

    virtual bool Init(void *win,XTextureType type)
    {
        mux.lock();
        sh.Close();
        this->type = type;
        if(!win) {
            mux.unlock();
            XLOGE("XTexture Init failed win is NULL");
            return false;
        }
        m_pView = (__bridge OpenGLView20*)win;
        [m_pView Init];
        sh.Init((XShaderType)type);
        mux.unlock();
        return true;
    }

    void surfaceChange(int width, int height)
    {
        ;
    }

    void ClearSurface()
    {
        [m_pView ClearFrame];
    }

    virtual void Draw(unsigned char *data[],int width,int height)
    {
        mux.lock();
        sh.GetTexture(0,width,height,data[0]);  // Y

        if(type == XTEXTURE_YUV420P)
        {
            sh.GetTexture(1,width/2,height/2,data[1]);  // U
            sh.GetTexture(2,width/2,height/2,data[2]);  // V
        }
        else
        {
            sh.GetTexture(1,width/2,height/2,data[1], true);  // UV
        }
        sh.Draw();
        [m_pView Draw];
        mux.unlock();
    }
};

XTexture *XTexture::Create()
{
    return  new CATexture();
}
