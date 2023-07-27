#include "avs_decoder.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "as_log.h"
#include "as_config.h"
#include "avs_player_factory.h"
#include <thread>
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "extend/as_frame.h"
#include <Windows.h>
#include <fstream>
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "../../../Prj-Android/allplayer/src/main/cpp/AndroidLog.h"
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_IOS)
#include "as_frame.h"
#endif

int CodecParasCopy(AVCodecParameters* dst, const MK_CodecParameters* src) {
    if (!dst || !src) {
        return AVERROR(EINVAL);
    }
    
    AVDictionary* options = nullptr;
    auto cpus = std::thread::hardware_concurrency();

    av_freep(&dst->extradata);
    memset(dst, 0, sizeof(*dst));

    dst->codec_type = (AVMediaType)src->codec_type;
    dst->codec_id = (AVCodecID)src->codec_id;
    dst->format = src->format;
    dst->width = src->width;
    dst->height = src->height;
    dst->field_order = AV_FIELD_UNKNOWN;
    dst->color_range = AVCOL_RANGE_UNSPECIFIED;
    dst->color_primaries = AVCOL_PRI_UNSPECIFIED;
    dst->color_trc = AVCOL_TRC_UNSPECIFIED;
    dst->color_space = AVCOL_SPC_UNSPECIFIED;
    dst->chroma_location = AVCHROMA_LOC_UNSPECIFIED;
    dst->sample_aspect_ratio = { 0, 1 };
    dst->profile = FF_PROFILE_UNKNOWN;
    dst->level = FF_LEVEL_UNKNOWN;

    dst->channels = src->channels;
    dst->sample_rate = src->sample_rate;

    dst->extradata = NULL;
    dst->extradata_size = 0;
    if (src->extradata) {
        dst->extradata = (uint8_t*)av_mallocz(src->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!dst->extradata)
            return AVERROR(ENOMEM);
        memcpy(dst->extradata, src->extradata, src->extradata_size);
        dst->extradata_size = src->extradata_size;
    }
    return 0;
}

AvsDecoder::~AvsDecoder(){
    close();
}

int AvsDecoder::probeHwAcc(AVCodecID codecId) {
    int ret = 0;
    AVCodec* codec = avcodec_find_decoder(codecId);
    if (!codec) {
        return AVERROR(ENOTSUP);
    }

    for (int i = 0;; i++) {
        const AVCodecHWConfig* hwcodec = avcodec_get_hw_config(codec, i);
        if (hwcodec == NULL) {
            break;
        }

        // 可能一个解码器对应着多个硬件加速方式，所以这里将其挑选出来
        if ((hwcodec->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
            (AVHWDeviceType::AV_HWDEVICE_TYPE_D3D11VA == hwcodec->device_type)) {
            m_hWPixelFmt = hwcodec->pix_fmt;
            break;
        }
    }

    if (AVPixelFormat::AV_PIX_FMT_NONE == m_hWPixelFmt) {
        return AVERROR(ENOTSUP);
    }

    // 启用硬件解码器
    ret = av_hwdevice_ctx_create(&m_hwCtx, AVHWDeviceType::AV_HWDEVICE_TYPE_D3D11VA, NULL, NULL, NULL);
    if (ret < 0 || !m_hwCtx) {
        char err_buf[FFMPEG_ERR_BUFF_LEN] = { 0 };
        av_strerror(ret, err_buf, sizeof(err_buf) - 1);
        AS_LOG(AS_LOG_ERROR, "d3d11va hwdevice context create failed, %s.", err_buf);
    }
    return ret;
}

int AvsDecoder::initFF(AVStream* stream, bool bHwAcc) {
    if (!stream || !stream->codecpar) {
        return -1;
    }
   
    int ret = 0;
    AVCodec* codec;
    char err_buf[FFMPEG_ERR_BUFF_LEN] = { 0 };
    if (!(codec = avcodec_find_decoder(stream->codecpar->codec_id))) {
        AS_LOG(AS_LOG_WARNING, "No decoder could be found for codec %s", avcodec_get_name(stream->codecpar->codec_id));
        ret = AVERROR(EINVAL);
        return ret;
    }

    std::unique_lock<std::mutex> lck(m_ctxMtx);
    avcodec_free_context(&m_codecCtx);
    if (!(m_codecCtx = avcodec_alloc_context3(codec))) {
        return AVERROR(ENOMEM);
    }
    
    do {
        if ((ret = avcodec_parameters_to_context(m_codecCtx, stream->codecpar)) < 0) {
            break;
        }

        m_codecCtx->pkt_timebase = stream->time_base;
        m_codecCtx->codec_id = codec->id;
        m_codecId = m_codecCtx->codec_id;

        if (bHwAcc) {
            if (!m_hwCtx) {
                if((ret = probeHwAcc(stream->codecpar->codec_id)) < 0) {
                    break;
                }
            }

            if (m_hwCtx) {
                m_codecCtx->hw_device_ctx = av_buffer_ref(m_hwCtx);
                av_buffer_unref(&m_hwCtx);
            }
        }

		AVDictionary* options = nullptr;
        if (AVMEDIA_TYPE_VIDEO == stream->codecpar->codec_type) {
            auto limit = avs_player_factory::GetInstance()->getDecoderThreadLimit();
            if (limit > 1) {
                auto thds = std::thread::hardware_concurrency();
                thds = std::min<int>(thds, limit);
                AS_LOG(AS_LOG_INFO, "set decode threads %d.", thds);
                if ((ret = av_dict_set(&options, "threads", std::to_string(thds).data(), 0)) < 0) {
                    break;
                }
            }
        }
        
		if ((ret = avcodec_open2(m_codecCtx, nullptr, &options)) < 0) {
			av_strerror(ret, err_buf, sizeof(err_buf) - 1);
			AS_LOG(AS_LOG_ERROR, "codec open failed, %s.", err_buf);
			break;
		}
    } while (0);

    if (ret < 0) {
        avcodec_free_context(&m_codecCtx);
        av_strerror(ret, err_buf, sizeof(err_buf) - 1);
        AS_LOG(AS_LOG_WARNING, "codec open failed, %s.", err_buf);
    }

    if (ret >= 0) {
        m_avStream = stream;
    }
    return ret;
}

int AvsDecoder::initMK(MK_Stream* stream, bool bHwAcc) {
    if (!stream || !stream->codecpar) {
        return AVERROR(EINVAL);
    }
    
    int ret = 0;
    MK_CodecParameters* para = stream->codecpar;
    auto codec = avcodec_find_decoder((AVCodecID)para->codec_id);
    if (!codec) {
        return AVERROR(ENOTSUP);
    }
	bool isVideo = (MEDIA_TYPE_VIDEO == para->codec_type);
    std::unique_lock<std::mutex> lck(m_ctxMtx);
 
    avcodec_free_context(&m_codecCtx);
    if (!(m_codecCtx = avcodec_alloc_context3(codec))) {
        return AVERROR(ENOMEM);
    }
   
    m_codecId = m_codecCtx->codec_id = codec->id;
    if (MEDIA_TYPE_AUDIO == para->codec_type) {
        m_codecCtx->sample_fmt = (AVSampleFormat)para->format;
        m_codecCtx->sample_rate = para->sample_rate;
        m_codecCtx->channels = para->channels;
        m_codecCtx->time_base.num = 1;
        m_codecCtx->time_base.den = para->sample_rate;
    }
    else if (MEDIA_TYPE_VIDEO == para->codec_type) {
        m_codecCtx->time_base.num = stream->time_base.num;
        m_codecCtx->time_base.den = stream->time_base.den;
    }

    do {
        if (bHwAcc) {
            if (!m_hwCtx) {
                if ((ret = probeHwAcc((AVCodecID)para->codec_id)) < 0)
                    break;
            }

            if (m_hwCtx) {
                m_codecCtx->hw_device_ctx = av_buffer_ref(m_hwCtx);
                av_buffer_unref(&m_hwCtx);
            }
        }
        AVCodecParameters ffpar;
        ffpar.extradata = NULL;
        CodecParasCopy(&ffpar, para);
        if ((ret = avcodec_parameters_to_context(m_codecCtx, &ffpar)) < 0) {
            break;
        }
		
		AVDictionary* options = nullptr;
		if (isVideo) {
            auto limit = avs_player_factory::GetInstance()->getDecoderThreadLimit();
            if (limit > 1) {
                auto thds = std::thread::hardware_concurrency();
                thds = std::min<int>(thds, limit);
                AS_LOG(AS_LOG_INFO, "set decode threads %d.", thds);
                if ((ret = av_dict_set(&options, "threads", std::to_string(thds).data(), 0)) < 0) {
                    break;
                }
            }
		}

		if ((ret = avcodec_open2(m_codecCtx, nullptr, &options)) < 0) {
			char errBuf[FFMPEG_ERR_BUFF_LEN] = { 0 };
			av_strerror(ret, errBuf, sizeof(errBuf) - 1);
			AS_LOG(AS_LOG_ERROR, "codec_open2 failed, %s.", errBuf);
			break;
		}
    } while (0);

    if (ret < 0) {
        avcodec_free_context(&m_codecCtx);
        char errBuf[FFMPEG_ERR_BUFF_LEN] = { 0 };
        av_strerror(ret, errBuf, sizeof(errBuf) - 1);
        AS_LOG(AS_LOG_ERROR, "codec open failed, %s.", errBuf);
	}

    if (ret >= 0) {
        m_mkStream = stream;
    }
    return ret;
}

int AvsDecoder::sendPkt(const AVPacket* pkt)
{
    std::unique_lock<std::mutex> lck(m_ctxMtx);
    if (!m_codecCtx) {
        return -1;
    }
    int ret = 0;
    if ((ret = avcodec_send_packet(m_codecCtx, pkt)) < 0) {
       /* char buf[FFMPEG_ERR_BUFF_LEN] = { 0 };
        av_strerror(ret,buf, FFMPEG_ERR_BUFF_LEN - 1);*/
    }
    return ret;
}

bool AvsDecoder::recvFrame(AVFrame* frame) {
    std::unique_lock<std::mutex> lck(m_ctxMtx);
    if (!m_codecCtx) {
        return false;
    }

    int ret = avcodec_receive_frame(m_codecCtx, frame);
    if (ret) {
        char buf[FFMPEG_ERR_BUFF_LEN];
        av_strerror(ret, buf, sizeof(buf) - 1);
    }
    return ret == 0;
}

int AvsDecoder::recvFrameInt(AVFrame* frame)
{
    std::unique_lock<std::mutex> lck(m_ctxMtx);
    if (!m_codecCtx) {
        AS_LOG(AS_LOG_WARNING, "codec has't init.");
        return AVERROR(EEXIST);
    }

    int ret = avcodec_receive_frame(m_codecCtx, frame);
    if (ret < 0) {
    }
    return ret;
}

void AvsDecoder::flushCodecBuffer()
{
    std::unique_lock<std::mutex> lck(m_ctxMtx);
    if (m_codecCtx) {
        avcodec_flush_buffers(m_codecCtx);
    }
}

bool AvsDecoder::saveFrameAsBMP(AVFrame* pFrame, std::string filePath)
{
#if ((AS_APP_OS & AS_OS_WIN) != AS_OS_WIN)
    return false;
#endif

    if (!m_codecCtx || !pFrame || !pFrame->height) {
        return false;
    }

    int ret = 0;
    AVFrame* srcFrame = pFrame;
    uint8_t* buffer = nullptr;

    if (AVPixelFormat::AV_PIX_FMT_NONE != m_hWPixelFmt && (pFrame->format == m_hWPixelFmt)) {
        AVFrame* sw_frame = av_frame_alloc();
        if (!sw_frame) {
            return false;
        }

        if (av_hwframe_transfer_data(sw_frame, pFrame, 0)) {
            av_frame_free(&sw_frame);
            return false;
        }
        srcFrame = sw_frame;
    }

    SwsContext* pSwsCtx = nullptr;

    do {
        pSwsCtx = sws_getContext(m_codecCtx->width, m_codecCtx->height, (AVPixelFormat)srcFrame->format,
            m_codecCtx->width, m_codecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

        if (!pSwsCtx) {
            ret = AVERROR(EINVAL);
            break;
        }

        if (!m_frameRGB && !(m_frameRGB = av_frame_alloc())) {
            ret = AVERROR(ENOMEM);
            break;
        }

        int nunBytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, m_codecCtx->width, m_codecCtx->height, 1);
        buffer = (uint8_t*)av_malloc(nunBytes * sizeof(uint8_t));
        if (!buffer) {
            ret = AVERROR(ENOMEM);
            break;
        }

        if ((ret = av_image_fill_arrays(m_frameRGB->data, m_frameRGB->linesize, buffer, AV_PIX_FMT_BGR24, m_codecCtx->width, m_codecCtx->height, 1)) < 0)
            break;

        if ((ret = sws_scale(pSwsCtx, srcFrame->data, srcFrame->linesize, 0, m_codecCtx->height, m_frameRGB->data, m_frameRGB->linesize)) < 0)
            break;
    } while (0);

    if (pSwsCtx) {
        sws_freeContext(pSwsCtx);
    }

    if (ret <= 0) {
        if (srcFrame != pFrame) {
            av_frame_free(&srcFrame);
        }
        av_frame_free(&m_frameRGB);
        if (buffer) {
            av_freep(&buffer);
        }
        return false;
    }

#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)

    BITMAPFILEHEADER bmpHeader = { 0 };
    //bmpHeader.bfType = ('M' << 8) | '8';
    bmpHeader.bfType = 0x4d42;
    bmpHeader.bfReserved1 = 0;
    bmpHeader.bfReserved2 = 0;
    bmpHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpHeader.bfSize = bmpHeader.bfOffBits + m_codecCtx->height * m_codecCtx->width * 3;

    BITMAPINFO bmpInfo = { 0 };
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = m_codecCtx->width;
    bmpInfo.bmiHeader.biHeight = -m_codecCtx->height;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 24;
    bmpInfo.bmiHeader.biCompression = 0;
    bmpInfo.bmiHeader.biSizeImage = 0;
    bmpInfo.bmiHeader.biXPelsPerMeter = 100;
    bmpInfo.bmiHeader.biYPelsPerMeter = 100;
    bmpInfo.bmiHeader.biClrUsed = 0;
    bmpInfo.bmiHeader.biClrImportant = 0;


    do {
        std::ofstream fout(filePath, std::ofstream::out | std::ofstream::binary);
        if (!fout) {
            ret = AVERROR(EINVAL);
            break;
        }

        fout.write(reinterpret_cast<const char*>(&bmpHeader), sizeof(BITMAPFILEHEADER));
        fout.write(reinterpret_cast<const char*>(&bmpInfo.bmiHeader), sizeof(BITMAPINFOHEADER));
        fout.write((const char*)m_frameRGB->data[0], m_codecCtx->height * m_codecCtx->width * 3);

    } while (0);

    if (srcFrame != pFrame) {
        av_frame_free(&srcFrame);
    }
    av_frame_free(&m_frameRGB);
    if (buffer) {
        av_freep(&buffer);
    }
    return ret >= 0;

#endif
    return true;
}

inline void freeTmpResource(AVFormatContext* formatCtx, AVStream* video_st, AVCodecContext* jpegCodecCtx, unsigned char* out_buffer)
{
    if (video_st) {
        avcodec_close(video_st->codec);
    }
    avio_close(formatCtx->pb);
    avcodec_free_context(&jpegCodecCtx);
    avformat_free_context(formatCtx);
    if (out_buffer) {
        av_freep(&out_buffer);
    }
}

bool AvsDecoder::saveFrameAsJpeg(AVFrame* pFrame, std::string filePath)
{
    if (!m_codecCtx || !pFrame || !pFrame->height) {
        return false;
    }

    AVFrame* srcFrame = pFrame;
    AVFormatContext* formatCtx = nullptr;
    AVStream* video_st = nullptr;
    AVCodec* jpegCodec = nullptr;
    AVCodecContext* jpegCodecCtx = nullptr;
    uint8_t* out_buffer = nullptr;
    char err_buf[FFMPEG_ERR_BUFF_LEN] = { 0 };

    // 分配AVFormatContext对象
    if (!(formatCtx = avformat_alloc_context())) {
        return false;
    }

    // 设置输出文件格式
    formatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
    if (avio_open(&formatCtx->pb, filePath.c_str(), AVIO_FLAG_READ_WRITE) < 0) {
        avformat_free_context(formatCtx);
        return false;
    }

    int ret = 0;
    do {
        if (!(video_st = avformat_new_stream(formatCtx, 0))) {
            ret = AVERROR(ENOMEM);
            break;
        }

        if (!(jpegCodec = avcodec_find_encoder(formatCtx->oformat->video_codec))) {
            ret = AVERROR(EINVAL);
            break;
        }

        if (!(jpegCodecCtx = avcodec_alloc_context3(jpegCodec))) {
            ret = AVERROR(ENOMEM);
            break;
        }
    } while (0);
    
    if (ret < 0) {
        av_strerror(ret, err_buf, sizeof(err_buf) - 1);
        AS_LOG(AS_LOG_WARNING, "save as jpg fail, %s", err_buf);

        if (video_st) {
            avcodec_close(video_st->codec);
        }
        avio_close(formatCtx->pb);
        avcodec_free_context(&jpegCodecCtx);
        avformat_free_context(formatCtx);
        av_freep(&out_buffer);
        return false;
    }

    AVPacket* packet = nullptr;
    do {
        if (!(srcFrame = transferFrameData(pFrame, &out_buffer))) {
            ret = AVERROR(EINVAL);
            break;
        }

        //jpegCodecCtx->codec_id = formatCtx->oformat->video_codec;
        //jpegCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        jpegCodecCtx->pix_fmt = AV_PIX_FMT_YUVJ420P;
        jpegCodecCtx->width = m_codecCtx->width;
        jpegCodecCtx->height = m_codecCtx->height;
        //time_base不设置,avcodec_open2返回-22
        jpegCodecCtx->time_base = { 1,25 };

        if((ret = avcodec_open2(jpegCodecCtx, jpegCodec, nullptr)) < 0)
            break;

        if ((ret = avcodec_parameters_from_context(video_st->codecpar, jpegCodecCtx)) < 0)
            break;
        
        if (!(packet = av_packet_alloc())) {
            ret = AVERROR(ENOMEM);
            break;
        }
        av_init_packet(packet);
        
        if((ret = avcodec_send_frame(jpegCodecCtx, srcFrame)) < 0)
            break;
        
        if((ret = avcodec_receive_packet(jpegCodecCtx, packet)) < 0)
            break;

        packet->stream_index = video_st->index;

        if((ret = avformat_write_header(formatCtx, NULL)) < 0)
            break;
        
        if ((ret = av_write_frame(formatCtx, packet)) < 0)
            break;

        if ((ret = av_write_trailer(formatCtx)) < 0)
            break;

    } while (0);

    if (ret < 0) {
        av_strerror(ret, err_buf, sizeof(err_buf) - 1);
        AS_LOG(AS_LOG_WARNING, "save as jpg fail, %s", err_buf);
    }

    freeTmpResource(formatCtx, video_st, jpegCodecCtx, out_buffer);
    if (srcFrame != pFrame) {
        av_frame_free(&srcFrame);
    }
    av_packet_free(&packet);
    return ret >= 0;
}

void AvsDecoder::close()
{
    std::unique_lock<std::mutex> lck(m_ctxMtx);
   
    av_frame_free(&m_frameRGB);
    if (m_codecCtx) {
        avcodec_free_context(&m_codecCtx);
    }
    if (m_hwCtx) {
        av_buffer_unref(&m_hwCtx);
    }
}

void AvsDecoder::clear(bool close) {
    std::unique_lock<std::mutex> lck(m_ctxMtx);
    if (m_codecCtx) {
        if (close) {
            lck.unlock();
            if (m_avStream) {
                initFF(m_avStream, !!m_hwCtx);
            }
            else if (m_mkStream) {
                initMK(m_mkStream, !!m_hwCtx);
            }
        }
        else {
            avcodec_flush_buffers(m_codecCtx);
        }
    }
}

bool  AvsDecoder::checkY(int iWidth, int iHeight, unsigned char* Buf) {
    if (iHeight <= 0 || iWidth <= 0 || !Buf) {
        return false;
    }
    
	unsigned char * pNewPoint = Buf;
	double iCountY = 0.0, iTotal = 0.0;

	//定位最后一行Y
	pNewPoint = Buf;
    int interVal = iHeight < 8 ? 1 : iHeight / 8 ;
    int line = 0;

    //遍历Y信息的所有高
    do {
        if (line == iHeight) {
            line = iHeight - 1;
        }
        unsigned char* pNewPoint2 = pNewPoint + line * iWidth;
        //遍历Y信息的宽
        for (int j = 0; j + 24 < iWidth; j += 24) {
            ++iTotal;
            int a = memcmp(pNewPoint2 + j, pNewPoint2 + j + 8, 8);
            int b = memcmp(pNewPoint2 + j + 8, pNewPoint2 + j + 16, 8);

            //判断连续两个8像素宏块，是否相同，并且在这个值范围内(0x7A~0x81)
            if (a == b && b == 0 && pNewPoint2[j] > 0x7A && pNewPoint2[j] <= 0x81) {
                iCountY++;
            }

            //判断绿屏
           if (a == b && 0 == b && 0x00 == pNewPoint2[j]) {
               iCountY++;
           }
        }
        line += interVal;
    }while (line <= iHeight);

    AS_LOG(AS_LOG_WARNING,"checY iCountY = %f, iTotal = %f, Percent = %f",iCountY,iTotal, iCountY / iTotal);
    if (iCountY/iTotal > 0.2) {
        return false;
    }
	return true;
}

AVFrame* AvsDecoder::transferFrameData(AVFrame* hwframe, uint8_t** src)
{
    if (AV_PIX_FMT_YUVJ420P == hwframe->format) {
        return hwframe;
    }

    AVFrame* sw_frame = hwframe;
    AVFrame* yuv_frame = nullptr;
    SwsContext* conversion_ctx = nullptr;
    if (!(yuv_frame = av_frame_alloc())) {
        return nullptr;
    }
       
    int ret = 0;
    char err_buf[FFMPEG_ERR_BUFF_LEN] = { 0 };

    do {
        if (hwframe->format == m_hWPixelFmt) {
            if (sw_frame = av_frame_alloc())
                break;

            if ((ret = av_hwframe_transfer_data(sw_frame, hwframe, 0)) < 0)
                break;
        }

        conversion_ctx = sws_getContext(
            hwframe->width, hwframe->height, (AVPixelFormat)sw_frame->format,
            hwframe->width, hwframe->height, AV_PIX_FMT_YUVJ420P,
            SWS_BICUBLIN | SWS_BITEXACT, nullptr, nullptr, nullptr);

        if (!conversion_ctx) {
            ret = -1;
            break;
        }

        *src = (uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, hwframe->width, hwframe->height, 1));
        if (!(*src)) {
            ret = AVERROR(ENOMEM);
            break;
        }

        if ((ret = av_image_fill_arrays(yuv_frame->data, yuv_frame->linesize, *src, AV_PIX_FMT_YUV420P, hwframe->width, hwframe->height, 1)) < 0) {
            break;
        }
        if ((ret = sws_scale(conversion_ctx, sw_frame->data, sw_frame->linesize, 0, sw_frame->height, yuv_frame->data, yuv_frame->linesize)) < 0) {
            break;
        }
    } while (0);

    if (ret < 0) {
        av_strerror(ret, err_buf, sizeof(err_buf) - 1);
        if (*src) {
            av_freep(src);
        }
        av_frame_free(&yuv_frame);
    }

    sws_freeContext(conversion_ctx);
    if (sw_frame != hwframe) {
        av_frame_free(&sw_frame);
    }
    if (yuv_frame) {
        yuv_frame->height = hwframe->height, yuv_frame->width = hwframe->width;
        yuv_frame->format = AV_PIX_FMT_YUVJ420P;
    }
    return yuv_frame;
}
