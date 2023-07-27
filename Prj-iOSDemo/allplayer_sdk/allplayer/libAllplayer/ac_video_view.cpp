#include "ac_video_view.h"
#include "sdl_video_view.h"

extern "C" {
#include<libavcodec/avcodec.h>
}

ACVideoView* ACVideoView::create(RenderType type)
{
    switch (type)
    {
    case ACVideoView::SDL:
        return new SDLVideoView();
    default:
        break;
    }
    return nullptr;
}

ACVideoView::ACVideoView()
{
}

bool ACVideoView::drawFrame(AVFrame* frame)
{
    if (!frame || !frame->data[0]) {
        return false;
    }

    switch (frame->format)
    {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
       return draw(frame->data[0], frame->linesize[0],//Y
            frame->data[1], frame->linesize[1],	//U
            frame->data[2], frame->linesize[2]	//V
        );
    default:
        break;
    }
    return true;
}
