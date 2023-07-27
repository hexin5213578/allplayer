#pragma once 

#include "avs_egress.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include "libavutil/audio_fifo.h"
}

#include "avs_resample.h"
#include "avs_decoder.h"
#include "avs_watermark.h"

// ��װ�������AVStream
typedef struct OutputStream {
    AVStream* st;               // ����һ��stream, 1·audio��1·video�����������steam
    AVAudioFifo* fifo;          //
    AvsResample* resampler;      // �ز�����
    AVCodecContext* enc;        // ������������

    AvsDecoder* dec;              // ������
    int last_duration;
    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;          // ��Ƶ�Ĳ��������ۼ�

    AVFrame* frame;             // �ز���ǰ��frame,��Ƶ��scale
    int input_index;
    uint8_t output_init;

    void reclaim() {
        if (enc) {
			avcodec_free_context(&enc);
		}
        last_duration = 0;
        next_pts = 0;
        output_init = 0;
    }

    void close() {
        if (fifo) {
            av_audio_fifo_free(fifo);
            fifo = nullptr;
        }
        AS_DELETE(resampler);
        AS_DELETE(dec);
        if (enc)    avcodec_free_context(&enc);
        if (frame)  av_frame_free(&frame);
        last_duration = 0;
        next_pts = 0;
        input_index = -1;
        output_init = 0;
    }

} OutputStream;

class AvsMuxer : public avs_egress
{
public:
    AvsMuxer(long bizId, int32_t bizType);
    virtual ~AvsMuxer();

    int init(std::string& output, MK_Format_Contex* format) override;
    
    int do_egress(AVPacket* pkt) override;
    
    bool waitForNextKey() override { return nullptr == m_waterMark; }

    int write_trailer() override;
	
    uint64_t getFileSize() override { return m_fileSize; }

    int64_t getDuration() override {
        return  m_videoSt.next_pts > 40 ? m_videoSt.next_pts - 40 : 0;
    }

    bool remuxing() { return !!m_pFormatCtx; }

    int32_t stop();

private:
    int32_t allocMuxerContext();

    int32_t processPkt(AVPacket* pkt);

    int32_t addStream(OutputStream* ost, MK_Stream* stream);

    int32_t decodeConvertStore(AVPacket* intputPacket);

    int32_t loadEncodeWrite();

    int32_t encodeAudioFrame(AVFrame* frame, int* dataPresent);
    
    int32_t encodeVideoFrame(AVFrame* frame);

    int32_t addWaterMark();
    
private:
    int32_t             m_resolution = 0;
    int64_t             m_lastVPts = INT64_MIN;
    AVFormatContext*    m_pFormatCtx = nullptr;

    OutputStream        m_videoSt;
    OutputStream        m_audioSt;
    bool                m_writeFrame = false;

    ACWaterMark*        m_waterMark = nullptr;
    uint64_t            m_fileSize = 0;
};
