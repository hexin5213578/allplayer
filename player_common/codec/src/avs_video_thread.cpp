#include "avs_video_thread.h"
#include "avs_player_common.h"
#include "as_config.h"
#include "avs_decoder.h"
#include "view_factory.h"
#include "avs_video_view.h"
#include "avs_video_filter.h"

#if ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "../../../Prj-Android/allplayer/src/main/cpp/AndroidLog.h"
#endif

extern "C"
{
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
}

const int MAX_PATH_LEN = 1024;
const int ERR_STR_LENGTH = 1024;
const int STEP_CAPCITY = 256;
const int DELTA_THRESHOLD = 3;
const int DOUBLE_FACTOR = 2;
const int PAUSE_INTERVAL = 10;
const int RESET_THRESHOLD = 5;
const int INTERVAL_THRESHOLD = 8;
const int CORRECT_TIME = 1;     //矫正时间 1ms


AvsVideoThread::AvsVideoThread(VideoState* ic)
    :AvsDecodeThread(ic) {

    m_waterMark = WaterMarkManager::get_instance()->produce_watermark(m_videoState->id);
 
    m_pktq = ic->videoq;
    if (!ic->pictq) {
        ic->pictq = make_shared<FrameQueue>(m_pktq, 3);
    }
    m_pktq->start();
    m_gops = make_shared<FrameGops>();
}

AvsVideoThread::~AvsVideoThread() {
    av_frame_free(&m_hwFrame);
}

bool AvsVideoThread::snapPicture(std::string filePath, uint8_t format) {
    auto framePtr = m_videoState->pictq->peekLast();
    auto frame = framePtr->frame;
    if (frame && frame->data) {
        //存在水印并且解码后没有添加，此时需要添加水印
        //if (m_waterMark && !WaterMarkManager::get_instance()->get_watermark_on(true)) {
        do {
            std::unique_lock<decltype(m_waterMutex)> lck(m_waterMutex);
            if (m_waterMark) {
                int ret1 = m_waterMark->filter_add_frame(frame);
                if (ret1 < 0 && AVERROR(EAGAIN) != ret1) {
                    return false;
                }

                ret1 = m_waterMark->filter_get_frame(frame);
                if (ret1 < 0) {
                    return false;
                }
            }
        } while (0);

        std::unique_lock<std::mutex> lock(m_ctxMutex);
        if (!m_decoder)  {
            return false;
        }
        AVFrame* frame1 = av_frame_alloc();
        if (!frame1) {
            return false;
        }
			
        if (av_frame_ref(frame1, frame) < 0) {
			av_frame_free(&frame1);
            return false;
        }
        
        bool ret = false;
        if (JPG == format) {
            ret = m_decoder->saveFrameAsJpeg(frame1, filePath);
        }
        else if (BMP == format) {
            ret = m_decoder->saveFrameAsBMP(frame1, filePath);
        }
        av_frame_free(&frame1);
        return ret;
    }
    return false;
}


void AvsVideoThread::captureFrames(std::string filePath, uint16_t count) {
    m_capturePath = filePath;
    m_captureCount = count;
}

int AvsVideoThread::adjustPictureParams(std::string& params) {
    std::unique_lock<std::mutex> lck(m_picFilterMtx);
    AS_DELETE(m_picFilter);
    m_picFilter = AS_NEW(m_picFilter);
    if (!m_picFilter || !m_picFilter->init(m_videoState->id)) {
        AS_DELETE(m_picFilter);
        return AVERROR(ENOMEM);
    }
    return m_picFilter->parse_filter_params(params);
}

std::string AvsVideoThread::getPictureParams() {
    std::unique_lock<std::mutex> lck(m_picFilterMtx);
    return m_picFilter ? m_picFilter->get_picture_params() : AvsVideoFilter::get_default_params();
}

int AvsVideoThread::probeHwAcc(MK_Stream* stream) {
    if (!stream || !stream->codecpar || !stream->codecpar->codec_id)
        return AVERROR(EINVAL);

    AVCodecID codecid = (AVCodecID)stream->codecpar->codec_id;
    if (!m_decoder && !(m_decoder = AS_NEW(m_decoder))) {
        return AS_ERROR_CODE_MEM;
    }
        
    int ret = 0;
    do {
        if ((ret = m_decoder->probeHwAcc(codecid)) < 0)
            break;

        if ((ret = m_decoder->initMK(stream, true)) < 0)
            break;
    } while (0);
    
    if (ret < 0) {
        m_decoder->close();
    }
    return ret;
}

int AvsVideoThread::setDecodePara(AVStream* stream,bool isHW) {
    std::unique_lock<std::mutex> lock(m_ctxMutex);
    if (m_decoder && !isHW) {
        m_decoder->close();
    }
    
    if (!m_decoder && !(m_decoder = AS_NEW(m_decoder))) {
        return AVERROR(ENOMEM);
    }
	
    int ret = m_decoder->initFF(stream, isHW);
	if(ret < 0) {
		AS_DELETE(m_decoder);
        return ret;
    }
    m_videoState->viddec = m_decoder;
    m_duration = stream->duration;
    m_hardWareAcc = isHW;
    return ret;
}

void AvsVideoThread::pushPacket(AVPacket* pkt)
{
    if (pkt) {
        m_pktq->packetPut(pkt);
    }
    else {
        m_pktq->packetqFlush();
    }
}

int AvsVideoThread::doTask(AVPacket* pkt)
{
    if (m_stepDirect < 0) {
        AS_LOG(AS_LOG_INFO, "biz[%ld] push packet when backward, pts %lld", m_videoState->id, pkt->pts);
    }

    m_pktq->packetPut(pkt);
    if (!m_videoState->cached_enough && (m_pktq->getDuation() >= (40 * m_videoState->pre_cache))) {
        m_videoState->cached_enough = true;
        return 1;
    } 

    //stay packet queue stable
    if (m_videoState->paused && m_videoState->realtime) {
        AVPacket pkt1;
        int s1;
        if (1 == m_pktq->packetGet(&pkt1, 0, &s1)) {
            av_packet_unref(&pkt1);
        }
    }
    return 0;
}

int AvsVideoThread::decoding() {
    int ret = 0;
    while (!m_bIsExit) {
        if(!m_videoState->cached_enough) {
            std::unique_lock<decltype(m_videoState->full_mutex)> lck(m_videoState->full_mutex);
            m_videoState->full_cond.wait_for(lck, std::chrono::milliseconds(100), [&] { return m_videoState->cached_enough; });
            continue;
        }

        if (getPaused()) {
            AVSleep(PAUSE_INTERVAL / 1000.0);
            continue;
        }

        ret = getVideoFrame();
        if (ret < 0) {
            AS_LOG(AS_LOG_ERROR, "decoder has exited or encounter unsolvable problems.");
            break;
        }
        
        if (!ret) {
            if (m_finished == m_serial) {
                m_videoState->sendStatus(STREAM_STATUS_EOS, "eos");
                m_arriveEOF = false;
            }
            AVSleep(0.01);
            continue;
        }

        processFrame(m_frame);
        av_frame_unref(m_frame);
    }
    return ret;
}

int AvsVideoThread::processFrame(AVFrame* frame) {
    if (!frame || NULL == frame->data[0] || 0 == frame->width || 0 == frame->height) {
        return AS_ERROR_CODE_AGAIN;
    }
    int ret = 0;
    AVFrame* useFrame = frame;

    do {
        AVPixelFormat format = m_decoder->getHWPixelFormat();
        m_hardWareAcc = (format == frame->format);

        if (m_hardWareAcc) {
            if (!m_hwFrame && !(m_hwFrame = av_frame_alloc())) {
                return AVERROR(ENOMEM);
            }
            auto ts = frame->pts;
            auto key = frame->key_frame;
            if ((ret = av_hwframe_transfer_data(m_hwFrame, frame, 0)) < 0) {
                return ret;
            }
            useFrame = m_hwFrame;
            useFrame->key_frame = key;
            useFrame->pts = ts;
        }
        
        std::unique_lock<decltype(m_waterMutex)> lck(m_waterMutex); 
        if(WaterMarkManager::get_instance()->get_watermark_on(true) && m_waterMark) {     
            ret = m_waterMark->filter_add_frame(useFrame);
            if (ret < 0 && AVERROR(EAGAIN) != ret)
                break;

            while (ret >= 0 && !m_bIsExit) {
                ret = m_waterMark->filter_get_frame(useFrame);
                /* if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;*/
                if (ret < 0) {
                    break;
                }
                if ((ret = applyPictureFilter(useFrame))) {
                    break;
                }
            }
        }
        else {
            lck.unlock();
            applyPictureFilter(useFrame);
        }
    } while (0);
    //unref 
    return ret;
}

int AvsVideoThread::getVideoFrame()
{
    int got_picture = 0;
    if ((got_picture = decodeFrame()) < 0) {
        return -1;
    }

    if (got_picture) {
        if (kNormal == m_videoState->play_mode) {
			if (m_threshold > 0) {
                if (m_frame->pts < m_threshold) {
                    return 0;
                }
                else {
                    m_threshold = 0;
                }
			}
        }
    }
    return got_picture;
}

int AvsVideoThread::applyPictureFilter(AVFrame* frame) {
    int ret = 0;
    std::unique_lock<std::mutex> lck(m_picFilterMtx);
    if (m_picFilter) {
        ret = m_picFilter->filter_add_frame(frame);
        if (ret < 0 && AVERROR(EAGAIN) != ret) {
            return ret;
		}

        while (ret >= 0 && !m_bIsExit) {
            ret = m_picFilter->filter_get_frame(frame);
            /* if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                 break;*/
            if (ret < 0) {
                break;
            }
            queueFrame(frame);
        }
    }
    else {
        queueFrame(frame);
    }

    implCapture();
    return ret;
}

int AvsVideoThread::queueFrame(AVFrame* frame1) {
    int ret = 0; 
    auto cache_frame = make_shared<MyFrame>();
    cache_frame->key_frame = frame1->key_frame;
    cache_frame->pts = frame1->pts / 1000.0;
    cache_frame->width = frame1->width;
    cache_frame->height = frame1->height;
    cache_frame->format = frame1->format;

    ret = av_frame_ref(cache_frame->frame, frame1);
    if (ret < 0) {
        AS_LOG(AS_LOG_WARNING, "frame ref error:%d.", ffmpeg_err(ret).data());
        return ret;
    }
    av_frame_unref(frame1);

    if (m_videoState->speed < 0.0) {
        m_gops->putMyFrame(cache_frame);
 
        auto factor = abs(m_videoState->speed);
        double duration = 0.0;

        if ((factor <= 2.0 && m_gops->getGopSize() > 2) ||               //-1,-2倍速，最少3个gop(防止第一个gop很小)
            (factor >= 4.0 && m_gops->getGopSize() > 4)) {              //-4倍速以上gop缓冲很快  Todo:适度放大
            FrameGop::Ptr gop = m_gops->m_reversalGops.front();
            AS_LOG(AS_LOG_WARNING, "put gop to render: %ld", gop->size());
            while (gop && !gop->isEmpty()) {
                auto ptr = gop->getMyFrame(true);
                if (ptr) {
                    putFrame2Render(ptr, 0);
                }
                gop->popFrame(true);
            }
            m_gops->m_reversalGops.pop_front();
            m_gops->setFirstGop();
        }
    }
    else  {
        ret = putFrame2Render(cache_frame, 1);
    }
    return ret;
}

void AvsVideoThread::implCapture() {
    if (m_captureCount && m_captured < m_captureCount) {
        const int MAX_PATH_LEN = 1024;
        char picture[MAX_PATH_LEN] = { 0 };
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
        sprintf_s(picture, MAX_PATH_LEN, "%s\\%lld_%d.jpg", m_capturePath.c_str(), getNowMs(), ++m_captured);
#else
        sprintf(picture, "%s\\%lld_%d.jpg", m_capturePath.c_str(), getNowMs(), ++m_captured);
#endif
        bool ret = snapPicture(picture, 0);
        m_videoState->sendStatus(ret ? CAPTURE_FRAME_SUCEESS : CAPTURE_FRAME_FAILED, picture);
        if (m_captureCount == m_captured) {
            m_captureCount = 0;
            m_captured = 0;
        }
    }
    return;
}

int AvsVideoThread::putFrame2Render(MyFrame::Ptr vp, int block, int timeout) {
    int ret = 0;
    static AVRational frame_rate = m_videoState->ic ? av_guess_frame_rate(m_videoState->ic, m_videoState->video_st, NULL) : AVRational{ 0,0 };
    if (vp) {
        double duration = (frame_rate.num && frame_rate.den ? av_q2d({ frame_rate.den, frame_rate.num }) : 0);
        vp->duration = duration;
        double time_base = m_videoState->video_st ? av_q2d(m_videoState->video_st->time_base) : 0.001;
        vp->pts = (vp->getSource()->pts == AV_NOPTS_VALUE) ? NAN : vp->getSource()->pts * time_base;
        vp->serial = m_serial;
        ret = m_videoState->pictq->pushFrame(vp, block);
    }
    else {
        AS_LOG(AS_LOG_INFO, "no free memory.");
        ret = -1;
    }
    return ret;
}

void AvsVideoThread::pause(bool isPause, bool clear) {
    AvsDecodeThread::pause(isPause, clear);
    m_stepDirect = 0;
}

void AvsVideoThread::clear() {
    m_pktq->packetqFlush();
    bool need_close = !m_gops->m_reversalGops.empty();
    if (m_decoder) {
        m_decoder->clear(need_close);
    }
    m_gops->clear();
    if (need_close) {
        std::unique_lock<decltype(m_waterMutex)> lck(m_waterMutex);
        AS_DELETE(m_waterMark);
        m_waterMark = WaterMarkManager::get_instance()->produce_watermark(m_videoState->id);
    }
}

int32_t AvsVideoThread::getStepFrameNB() {
    return m_pktq->packetSize();
}

void AvsVideoThread::step2NextFrame(int8_t stepDirect)
{
    m_stepDirect = stepDirect;
    if (m_paused) {
        AvsDecodeThread::pause(false, false);
    }
}

void AvsVideoThread::exitStep(bool isSeek) {
    m_stepDirect = 0;
    if (isSeek) {
        AvsDecodeThread::clear();
    }
}

void AvsVideoThread::stopRun() {
    AvsDecodeThread::stopRun();
    AS_DELETE(m_waterMark);
    AS_DELETE(m_picFilter);
}

void AvsVideoThread::restart() {
    AvsDecodeThread::restart();
    m_videoState->cached_enough = false;
}


