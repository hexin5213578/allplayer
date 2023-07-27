#include "avs_demuxer.h"
#include "avs_player_common.h"

AvsDemuxer::AvsDemuxer(VideoState* ic)
    :m_videoState(ic),
    m_pFmtCtx(nullptr)
{
    if (!m_videoState) {
        throw std::runtime_error("invalid video state");
    }
}

AvsDemuxer::~AvsDemuxer()
{
    close();
}

std::string AvsDemuxer::probeUrl(const std::string& url) {
    string url_info;
    int ret = 0;
    AVFormatContext* fmt = nullptr;

    auto root = cJSON_CreateObject();
    if (!root) {
        AS_LOG(AS_LOG_ERROR, "cJSON_CreateObject failed!");
        return url_info;
    }

    do {
        if ((ret = avformat_open_input(&fmt, url.c_str(), nullptr, nullptr)) < 0) {
            break;
        }

        if ((ret = avformat_find_stream_info(fmt, nullptr)) < 0) {
            break;
        }
    } while (0);

    if (ret < 0) {
        auto format = cJSON_CreateString("unsupport");
        cJSON_AddItemToObject(root, "format", format);
    }
    else {
        auto format = cJSON_CreateString(fmt->iformat->name);
        cJSON_AddItemToObject(root, "format", format);

        auto encrypt = cJSON_CreateNumber(0);
        cJSON_AddItemToObject(root, "encrypted", encrypt);

        auto duration = AV_NOPTS_VALUE == fmt->duration ? 0 : fmt->duration / 1000000LL;
        auto total = cJSON_CreateNumber(duration);
        cJSON_AddItemToObject(root, "duration", total);
    }
    auto info_str = cJSON_Print(root);
    url_info = std::string(info_str);
    free(info_str);
    cJSON_Delete(root);
    avformat_close_input(&fmt);
    return url_info;
}

int AvsDemuxer::openInput(const std::string& url)
{
    int ret = 0;
    char err[128] = { 0 };
    char* point = nullptr;

    do {
        if((ret = avformat_open_input(&m_pFmtCtx, url.c_str(), nullptr, nullptr)) < 0) {
            point = "open demuxer format";
            break;
        }

        if((ret = avformat_find_stream_info(m_pFmtCtx, nullptr)) < 0) {
            point = "find codec parameters";
            break;
        }

        m_videoState->ic = m_pFmtCtx;
        m_videoState->max_frame_duration = (m_pFmtCtx->iformat->flags & AVFMT_TS_DISCONT) ? 8.0 : 3600.0;

        if (!strcmp(m_pFmtCtx->iformat->name, "rtp")
            || !strcmp(m_pFmtCtx->iformat->name, "rtsp")
            || !strcmp(m_pFmtCtx->iformat->name, "sdp"))
            m_videoState->realtime = 1;
           
        if (m_pFmtCtx->pb && (!strncmp(m_pFmtCtx->url, "rtp:", 4) || !strncmp(m_pFmtCtx->url, "udp:", 4)))
            m_videoState->realtime = 1;
       
        int st_index[AVMEDIA_TYPE_NB];
        memset(st_index, -1, sizeof(st_index));

        st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(m_videoState->ic, AVMEDIA_TYPE_VIDEO, 
            st_index[AVMEDIA_TYPE_VIDEO], -1, nullptr, 0);

        st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(m_videoState->ic, AVMEDIA_TYPE_AUDIO,
                st_index[AVMEDIA_TYPE_AUDIO],
                st_index[AVMEDIA_TYPE_VIDEO],
                nullptr, 0);
        
        m_videoState->video_stream = st_index[AVMEDIA_TYPE_VIDEO];
        m_videoState->audio_stream = st_index[AVMEDIA_TYPE_AUDIO];

        if (m_videoState->video_stream < 0) {
            ret = m_videoState->video_stream;
            break;
        }

        m_videoState->video_st = m_videoState->ic->streams[m_videoState->video_stream];
        if (m_videoState->audio_stream >= 0) {
            m_videoState->audio_st = m_videoState->ic->streams[m_videoState->audio_stream];
        }
    } while (0);
   
    if (ret < 0) {
        av_strerror(ret, err, sizeof(err) - 1);
        AS_LOG(AS_LOG_WARNING, "open input %s fail, error happens in %s, %s.", url.c_str(), point, err);
        avformat_close_input(&m_pFmtCtx);
    }
    return ret;
}

int AvsDemuxer::read(AVPacket* pkt)
{
    std::unique_lock<std::mutex> lck(m_ctxMtx);
    if (!m_pFmtCtx){
        return AVERROR(EINVAL);
    }
    //pkt必须unref
    int ret = av_read_frame(m_pFmtCtx, pkt);
    return ret;
}

int AvsDemuxer::seek(double pos, MEDIA type)
{
    if (UNKOWN == type)
        return AVERROR(EINVAL);

    std::unique_lock<std::mutex> lck(m_ctxMtx);
    if (!m_pFmtCtx) {
        return AVERROR(EINVAL);
    }

    int idx = (VIDEO == type ? m_videoState->video_stream : m_videoState->audio_stream);
    if (idx < 0) {
        return AVERROR(EINVAL);
    }

    int64_t totalPts = m_pFmtCtx->streams[idx]->duration;// * av_q2d(m_pFmtCtx->streams[idx]->time_base);
    int64_t pts = totalPts * pos;
    int ret = av_seek_frame(m_pFmtCtx, idx, pts, AVSEEK_FLAG_FRAME | AVSEEK_FLAG_BACKWARD);
    if (ret < 0)
    {
        char buf[128] = { 0 };
        av_strerror(ret, buf, sizeof(buf) - 1);
        AS_LOG(AS_LOG_WARNING, "demux seek to %lf fail, %s.", pos, ret);
    }
    return ret;
}

void AvsDemuxer::close()
{
    if (m_pFmtCtx) {
        if (m_pFmtCtx->pb) {
            av_freep(&(m_pFmtCtx->pb->buffer));
            avio_context_free(&m_pFmtCtx->pb);
        }
        avformat_close_input(&m_pFmtCtx);
    }
}

