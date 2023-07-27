#include "ac_texture.h"
#include "ac_EGL.h"
#include "ac_shader.h"
#include "app_log.h"

class CXTexture:public XTexture
{
public:
    ac_shader sh;
    XTextureType type;
    std::mutex mux;
    XEGL* m_pEGL = nullptr;

    virtual void Drop()
    {
        std::unique_lock<std::mutex> lck(mux);
        sh.Close();
        if(m_pEGL) {
            m_pEGL->Close();
            delete m_pEGL;
        }
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
        m_pEGL = XEGL::Create();
        if(!m_pEGL->Init(win)) {
            mux.unlock();
            return false;
        }
        sh.Init((XShaderType)type);
        mux.unlock();
        return true;
    }

    void surfaceChange(int width, int height)
    {
        m_pEGL->Change(width,height);
    }

    void ClearSurface()
    {
        m_pEGL->ClearSurface();
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
        m_pEGL->Draw();
        mux.unlock();
    }
};

XTexture *XTexture::Create()
{
    return  new CXTexture();
}
