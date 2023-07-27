#ifndef AC_PKT_FRAME_Q_H__
#define AC_PKT_FRAME_Q_H__

#include "util.h"
#include "as_log.h"
#include "List.h"

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
}

class MyAVPacket {
public:
    using Ptr = std::shared_ptr<MyAVPacket>;

    MyAVPacket() = default;
    virtual ~MyAVPacket() = default;

    AVPacket pkt;
    int serial = 0;
};

class PacketQueue;
class PktGop {
public:
    using Ptr = std::shared_ptr<PktGop>;
    PktGop(std::weak_ptr<PacketQueue> pktq) { m_pktq = pktq;  }

    int putAVPkt(AVPacket* pkt);
    MyAVPacket::Ptr getAVPkt();

    bool isStart() { return m_have_idr; }
    int64_t getFirstPts() { 
        if (m_gop.empty()) return 0;
        return m_gop.front()->pkt.pts;
    }

    int64_t getLastPts() {
        if (m_gop.empty()) return 0;
        return m_gop.back()->pkt.pts;
    }

    size_t getSize() { return m_gop.size(); }

    int64_t getDuration();

    void clear();

private:
    avs_toolkit::List<MyAVPacket::Ptr> m_gop;
    bool m_have_idr = false;
    std::weak_ptr<PacketQueue>   m_pktq;
};

class PacketQueue : public std::enable_shared_from_this<PacketQueue> {
public:
    using Ptr = std::shared_ptr<PacketQueue>;
    PacketQueue();
    virtual ~PacketQueue();

    void start();
    void abort();
    int serial() { return m_serial; }
    int getAbort() { return m_abortRequest; }

    /* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
    int packetGet(AVPacket* pkt, int block, int* serial);
    int packetPut(AVPacket* pkt);
    int putNullPacket(int stream_index);
    int packetSize();
    int64_t getDuation();
    void packetqFlush();

    int putGop(PktGop::Ptr);
    int popFrontGop();

    int     m_serial = 0;

private:
    int packetPutPrivate(AVPacket* pkt);

    avs_toolkit::List<PktGop::Ptr>  m_gops;
    int                         m_abortRequest = 1;
    int                         m_maxGop = 3;
    std::condition_variable     m_cond;
    std::mutex        m_mutex;
};

class MyFrame {
public:
    using Ptr = std::shared_ptr<MyFrame>;
    MyFrame() {
        frame = av_frame_alloc();
    }

    virtual ~MyFrame() {
        av_frame_free(&frame);
    }

    AVFrame* getSource() {
        return frame;
    }

    MyFrame(const MyFrame&) = delete;
    MyFrame& operator=(const MyFrame&) = delete;

    AVFrame* frame = nullptr;
    int serial = 0;
    double pts = 0.0;
    double duration = 0.0;
    int width = 0;
    int height = 0;
    int format = 0;
    bool key_frame;
};

class FrameGops;
class FrameGop {
public:
    friend class FrameGops;
    using Ptr = std::shared_ptr<FrameGop>;
    FrameGop() = default;
    ~FrameGop();

    int putMyFrame(MyFrame::Ptr frame);
    MyFrame::Ptr getMyFrame(bool reversal);
    void popFrame(bool reversal);

    size_t size() {
        return m_gop.size();
    }

    bool isStarted() { return m_have_idr; }

    bool isEmpty() {
        return m_gop.empty();
    }

    double getDuration();

    double getFirstPts() {
        if (m_gop.empty()) return 0.0;
        return m_gop.front()->pts;
    }

    double getLastPts() {
        if (m_gop.empty())  return 0.0;
        return m_gop.back()->pts;
    }

private:
    avs_toolkit::List<MyFrame::Ptr> m_gop;
    int64_t m_pts = -1;
    bool m_have_idr = false;
};

class FrameGops {
public:
    using Ptr = std::shared_ptr<FrameGops>;
    virtual ~FrameGops() {
        clear();
    }

    double getDuration();
    
    int getGopSize() {
        if (m_reversalGops.empty()) {
            return 0;
        }
        return m_reversalGops.size() - firstGop;
    }

    int getFrameSize() {
        return m_framSize;
    }

    int putMyFrame(MyFrame::Ptr frame);

    void clear() {
        m_reversalGops.clear();
        m_framSize = 0;
        firstGop = 1;
    }

    //��һ�����ܲ�������gop�Ѿ����Ƴ�
    void setFirstGop() {
        firstGop = 0;
    }

    int firstGop = 1;
    int m_framSize = 0;
    std::list<FrameGop::Ptr >	m_reversalGops;
};



class FrameQueue : public std::enable_shared_from_this<FrameQueue> {
public:
    using Ptr = std::shared_ptr<FrameQueue>;

    FrameQueue(PacketQueue::Ptr pktq, size_t maxSize) : m_pktq(pktq), m_maxSize(maxSize) {
    }

    virtual ~FrameQueue() {
        m_queue.clear();
    }

    void signal() {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_cond.notify_all();
    }

    // Add frame to the end of the queue
    int pushFrame(const MyFrame::Ptr frame, int block, int timeout = 1000) {
        std::unique_lock<std::mutex> lck(m_mtx);
        if (block) {
            auto self = shared_from_this();
            m_cond.wait_for(lck, std::chrono::milliseconds(timeout), [self] {
                auto pktq = self->m_pktq.lock();
                if (!pktq || pktq->getAbort()) {
                    return true;
                }

                //block��Ҫ�жϿռ��Ƿ��㹻
                if (self->m_queue.size() < self->m_maxSize) {
                    return true;
                }
                return false;
            });
        }

        auto pktq = m_pktq.lock();
        if (!pktq || pktq->getAbort()) {
            return -1;
        }

        if (block && m_queue.size() >= m_maxSize) {
            AS_LOG(AS_LOG_WARNING, "FrameQueue is full, pushFrmae trigger timeout.");
            return -1;
        }

        m_queue.emplace_back(frame);
        m_cond.notify_all();
        return 0;
    }

    // Get frame from the front of the queue
    void queueNext(int timeout = 100) {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_cond.wait_for(lck, std::chrono::milliseconds(timeout), [&] { return !m_queue.empty(); });
        if (m_queue.empty()) {
            return;
        }
        m_lastFrame = m_queue.front();
        m_queue.pop_front();
        m_cond.notify_all();
        return;
    }

    // Peek the last frame taken
    MyFrame::Ptr peekLast() {
        return m_lastFrame ? m_lastFrame : m_queue.front();
    }

    // Peek the first frame in the queue
    MyFrame::Ptr peek() {
        if (!m_queue.empty()) {
            return m_queue.front();
        }
        return nullptr;
    }

    // Peek the second frame in the queue
    MyFrame::Ptr peekNext() {
        std::lock_guard<std::mutex> lck(m_mtx);
        if (m_queue.size() > 1) {
            return *(std::next(m_queue.begin()));
        }
        return nullptr;
    }

    // Get the current size of the buffer
    size_t remainingNb() {
        std::lock_guard<std::mutex> lck(m_mtx);
        return m_queue.size();
    }

    // Check if there's a last frame
    bool rShown() {
        return !!m_lastFrame;
    }

private:
    size_t m_maxSize;
    std::deque<MyFrame::Ptr> m_queue;
    MyFrame::Ptr m_lastFrame;
    std::mutex m_mtx;
    std::condition_variable m_cond;
    std::weak_ptr<PacketQueue>  m_pktq;
};

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE 51 // FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

#endif // !AC_PKT_FRAME_Q_H__