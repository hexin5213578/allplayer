#ifndef XPLAY_XEGL_H
#define XPLAY_XEGL_H

class XEGL
{
public:
    virtual bool Init(void *win) = 0;
    virtual void Close() = 0;
    virtual void Draw() = 0;
    virtual void Change(int width,int height) = 0;
    virtual void ClearSurface() = 0;
    static XEGL* Create();

protected:
    XEGL(){}
};

#endif //XPLAY_XEGL_H
