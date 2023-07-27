#ifndef __AVS_EGRESS_H__
#define __AVS_EGRESS_H__

#include "avs_player_common.h"

#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
}

struct AVPacket;

struct pktsCache
{
    std::queue<AVPacket*> pkts;
    std::mutex mtx;

    pktsCache() = default;
    virtual ~pktsCache() {
        std::unique_lock<std::mutex> lck(mtx);
        while (!pkts.empty()) {
            AVPacket* pkt = pkts.front();
            pkts.pop();
            av_packet_free(&pkt);
        }
    }

    void push(AVPacket* pkt) {
        AVPacket* muxPkt = nullptr;
        if (!pkt || !(muxPkt = av_packet_alloc())) {
            return;
        }
    
        if ((0 == pkt->size) && (pkt == (void*)pkt->data)) {
            muxPkt->size = 0;
            muxPkt->data = (uint8_t*)muxPkt;
        }
        else if (av_packet_ref(muxPkt, pkt) < 0) {
            av_packet_free(&muxPkt);
            return;
        }
        std::unique_lock<std::mutex> lck(mtx);
        pkts.push(muxPkt);
    }

    AVPacket* pop() {
        std::unique_lock<std::mutex> lck(mtx);
        if (!pkts.empty()) {
            AVPacket* pkt = pkts.front();
            pkts.pop();
            return pkt;
        }
        return nullptr;
    }

    int size() {
        std::unique_lock<std::mutex> lck(mtx);
        return pkts.size();
    }
};

class avs_egress
{
public:
	avs_egress(long id, int type):m_bizId(id), m_bizType(type),
		m_format(nullptr)
	{
	}
    virtual ~avs_egress() = default;

	virtual int init(std::string& output, MK_Format_Contex* format) = 0;
	virtual int do_egress(AVPacket* pkt) = 0;
    
    virtual uint64_t getFileSize() = 0;
    virtual int64_t getDuration() = 0;
    virtual bool remuxing() = 0;

	virtual bool waitForNextKey() { return true; }  //是否需要等待下一个关键帧,没有水印时需要
	virtual int write_trailer() = 0;
	virtual int stop() = 0;

protected:
	long				m_bizId;
	int					m_bizType;

	MK_Format_Contex*	m_format;
	std::string         m_outPath;
};

#endif /* __AVS_EGRESS_H__ */
