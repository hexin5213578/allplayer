#include "avs_video_view.h"
extern "C" 
{
#include<libavcodec/avcodec.h>
}

AvsVideoView::AvsVideoView(void* hwnd, ViewType type):
    m_hWnd(hwnd), m_type(type) {
    memset(m_datas, 0, sizeof(m_datas));
}

bool AvsVideoView::drawFrame(AVFrame* frame)
{
    if (!frame || !frame->data[0]) {
        return false;
    }

    int linesize;
    switch (frame->format) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
       return draw(frame->data[0], frame->linesize[0],//Y
            frame->data[1], frame->linesize[1],	//U
            frame->data[2], frame->linesize[2]	//V
        );
    case AV_PIX_FMT_NV12:
        if (m_cache.capacity() < frame->width * frame->height * 2) {
            return false;
        }
        linesize = frame->width;
        if (frame->linesize[0] == frame->width) {
            memcpy(m_cache.data(), frame->data[0], frame->linesize[0] * frame->height);
            memcpy(m_cache.data() + frame->linesize[0] * frame->height, frame->data[1], frame->linesize[1] * frame->height / 2);
        }
        else {
            for (int i = 0; i < frame->height; i++) {
                memcpy(m_cache.data() + i * frame->width, frame->data[0] + i * frame->linesize[0], frame->width);
            }

            auto p = m_cache.data() + frame->width * frame->height;
            for (int i = 0; i < frame->height / 2; i++) {
                memcpy(p + i * frame->width, frame->data[1] + i * frame->linesize[1], frame->width);
            }
        }
        draw(m_cache.data(), linesize);
        break;
    case AV_PIX_FMT_D3D11:
        return draw(frame->data,frame->width,frame->height);
        break;
    default:
        break;
    }
    return true;
}

bool AvsVideoView::drawFrame(AVFrame *frame, int width, int height) {

    std::unique_lock<std::mutex> lck(m_dataMtx);
    if (frame->width != frame->linesize[0]) {
        if(!m_bAllocated) {
            if(m_datas[0]) {
                delete[] m_datas[0], delete[] m_datas[1], delete[] m_datas[2];
            }
            m_datas[0] = new uint8_t[frame->width * frame->height];        //Y
            m_datas[1] = new uint8_t[frame->width * frame->height / 4];    //U
            m_datas[2] = new uint8_t[frame->width * frame->height / 4];    //V
            m_bAllocated = true;
        }
        for (int i = 0; i < frame->height; ++i) {
            memcpy(m_datas[0] + frame->width * i, frame->data[0] + i * frame->linesize[0],
                   frame->width);
        }
        for (int i = 0; i < frame->height / 2; ++i) {
            memcpy(m_datas[1] + frame->width * i / 2, frame->data[1] + i * frame->linesize[1],
                   frame->width/2);
        }
        for (int i = 0; i < frame->height / 2; ++i) {
            memcpy(m_datas[2] + i * frame->width / 2, frame->data[2] + i * frame->linesize[2],
                   frame->width/2);
        }
    }

    if(m_surfaceWidth != width || m_surfaceHeight != height) {
        return draw(m_bAllocated ? m_datas : frame->data,frame->width,frame->height,width,height);
    }
    else {
        return draw(m_bAllocated ? m_datas : frame->data,frame->width,frame->height);
    }
}

void AvsVideoView::setMarginBlankRatio(int iTop, int iRight, int iBottom, int iLeft)
{
	if (m_iTop != iTop || m_iRight != iRight || m_iBottom != iBottom || m_iLeft != iLeft) {
		m_iTop = iTop;
		m_iRight = iRight;
		m_iBottom = iBottom;
		m_iLeft = iLeft;
	}
}

AvsVideoView::~AvsVideoView() {
    std::unique_lock<std::mutex> lck(m_dataMtx);
    if(m_bAllocated) {
		delete[] m_datas[0];
		delete[] m_datas[1];
		delete[] m_datas[2];
    }
    m_cache.clear();
}
