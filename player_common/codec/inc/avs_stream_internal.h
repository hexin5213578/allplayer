#pragma once

#include <vector>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
}

#include "libMediaService.h"

class AvsStreamInternal
{
public:
    AvsStreamInternal(MK_Stream* st)
        :stream(st) {
    }
    virtual ~AvsStreamInternal() = default;

    int init();

    void release();

    int gen_packet(AVPacket* pkt, const uint8_t *buf, int len);

    int parse_packet(AVPacket* pkt);

private:
    MK_Stream* stream = nullptr;
    AVCodecContext* avctx = nullptr;
    struct AVCodecParserContext* parser = nullptr;
    AVPacket* parse_pkt = nullptr;
    std::vector<AVPacket*> parse_queue;
};