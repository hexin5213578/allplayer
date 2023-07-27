#include "avs_pkt_frame_q.h"

using namespace std;

static const int kthreshold = 512;

int PktGop::putAVPkt(AVPacket* pkt) {
    if (!pkt) {
        return AVERROR(EINVAL);
    }
    
    auto pkt1 = make_shared<MyAVPacket>();
    if (!pkt1) {
        return AVERROR(ENOMEM);
    }

    int ret = 0;
    bool key_frame = pkt->flags & AV_PKT_FLAG_KEY;
    auto pts = pkt->pts;
    if (!pkt->data && !pkt->size) {   // eof pkt, to trigger receive eof
        pkt1->pkt = *pkt;
    }
    else {
        AVPacket* pkt2 = &pkt1->pkt;
        av_init_packet(pkt2);
        //���ü��� �������ݸ��ƣ��̰߳�ȫ
        if ((ret = av_packet_ref(pkt2, pkt)) < 0) {
            return ret;
        }
        pkt1->pkt.pts = pkt1->pkt.pts = pts;
    }

    if (auto pktq = m_pktq.lock()) {
        pkt1->serial = pktq->serial();
    }
    if (key_frame) {
        m_have_idr = true;
    }

    m_gop.emplace_back(pkt1);
    return ret;
}

MyAVPacket::Ptr PktGop::getAVPkt() {
    if (m_gop.empty()) {
        return nullptr;
    }
    MyAVPacket::Ptr ret = m_gop.front();
    m_gop.pop_front();
    return ret;
}

int64_t PktGop::getDuration() {
    auto sz = m_gop.size();
    auto start = m_gop.front();
    auto end = m_gop.back();

    if (start->serial != end->serial) {
        AS_LOG(AS_LOG_WARNING, "gop has different serial.");
        return 0;
    }
    else {
        auto total = end->pkt.pts - start->pkt.pts;
        return total / sz * (sz + 1);
    }
    return 0;
}

void PktGop::clear() {
    for (auto& pkt : m_gop) {
        av_packet_unref(&pkt->pkt);
    }
    m_gop.clear();
}

/////////////////////////////////////////////////////////////////////////

PacketQueue::PacketQueue() {
    //m_maxPktNb = kthreshold;
}

PacketQueue::~PacketQueue() {
    packetqFlush();
}

int PacketQueue::packetGet(AVPacket* pkt, int block, int* serial) {
    int ret = 0;
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);

    for (;;) {
        if (m_abortRequest) {
            ret = -1;
            break;
        }

        while (!m_gops.empty()) {
            auto gop = m_gops.front();
            auto pkt1 = gop->getAVPkt();
            if (!pkt1) {
                ret = 0;
                m_gops.pop_front();
                continue;
            }
           
            *pkt = pkt1->pkt;
            if (serial) {
                *serial = pkt1->serial;
            }
            ret = 1;
            break; 
        }

        if (ret > 0) {
            break;
        }

        if (!block) {
            ret = 0;
            break;
        }
        else {
            if (m_gops.size() > 0) {
                PrintW("thread block while gop size: %d.", m_gops.size());
            }
            m_cond.wait(lock);
        }
    }
    return ret;
}

int PacketQueue::packetPut(AVPacket* pkt) {
    int ret = packetPutPrivate(pkt);

    if (packetSize() > kthreshold) {
        std::unique_lock<decltype(m_mutex)> lck(m_mutex);
        while (m_gops.size() > 1) {
            m_gops.pop_front();
        }
        AS_LOG(AS_LOG_WARNING, "clear pakcets while performance insufficient.");
        this->m_serial++;
    }
    return ret;
}

int PacketQueue::putNullPacket(int stream_index) {
    AVPacket pkt1, * pkt = &pkt1;
    av_init_packet(pkt);
    pkt->data = nullptr;
    pkt->size = 0;
    pkt->stream_index = stream_index;
    return packetPut(pkt);
}

int PacketQueue::packetSize() {
    int size = 0;
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    for (auto& gop : m_gops) {
        size += gop->getSize();
    }
    return size;
}

int64_t PacketQueue::getDuation() {
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    if (m_gops.empty()) {
        return 0;
    }
    int64_t dur = m_gops.back()->getLastPts() - m_gops.front()->getFirstPts() + 40;
    return dur;
}

void PacketQueue::packetqFlush() {
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    for (auto& gop : m_gops) {
        gop->clear();
    }
    m_gops.clear();
    this->m_serial++;
}

void PacketQueue::start() {
    do {
        std::unique_lock<decltype(m_mutex)> lock(m_mutex);
        m_abortRequest = 0;
        ++this->m_serial;
    } while (0);
}

void PacketQueue::abort() {
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    m_abortRequest = 1;
    lock.unlock();
    m_cond.notify_one();
}

int PacketQueue::putGop(PktGop::Ptr gop) {
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    m_gops.emplace_back(gop);
    return 0;
}

int PacketQueue::popFrontGop() {
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);
    if (!m_gops.empty()) {
        m_gops.pop_front();
    }
    return 0;
}

int PacketQueue::packetPutPrivate(AVPacket* pkt) {
    if (m_abortRequest) {
        return -1;
    }

    bool key_frame = pkt->flags & AV_PKT_FLAG_KEY;
    std::unique_lock<decltype(m_mutex)> lock(m_mutex);

    if (key_frame || m_gops.empty()) {
        auto gop = make_shared<PktGop>(shared_from_this());
        gop->putAVPkt(pkt);
        m_gops.emplace_back(gop);
    }
    else {
        auto& gop = m_gops.back();
        gop->putAVPkt(pkt);
    }  
    m_cond.notify_one();
    return 0;
}

/////////////////////////////////////////////////////////////////////////////

FrameGop::~FrameGop() {
    m_gop.clear();
}

int FrameGop::putMyFrame(MyFrame::Ptr frame) {
    if (!frame || !frame->frame) {
        return AVERROR(EINVAL);
    }
    
    //std::unique_lock<decltype(m_mtx)> lck(m_mtx);
    if (frame->key_frame) {
        m_have_idr = true;
        m_pts = frame->pts;
    }
    m_gop.emplace_back(frame);
    return 0;
}

MyFrame::Ptr FrameGop::getMyFrame(bool reversal) {
    //std::unique_lock<decltype(m_mtx)> lck(m_mtx);
    if (m_gop.empty()) {
        return nullptr;
    }
    
    if (reversal) {
        MyFrame::Ptr frame = m_gop.back();
        return frame;
    }
    else {
        MyFrame::Ptr frame = m_gop.front();
        return frame;
    }
}

void FrameGop::popFrame(bool reversal) {
    //std::unique_lock<decltype(m_mtx)> lck(m_mtx);
    if (m_gop.empty()) {
        return;
    }

    if (reversal) {
        m_gop.pop_back();
    }
    else {
        m_gop.pop_front();
    }
}

double FrameGop::getDuration() {
    if (m_gop.empty()) {
        return 0;
    }

    auto duration = abs(m_gop.back()->pts - m_gop.front()->pts);
    return duration + 0.04;
}

double FrameGops::getDuration() {
    if (m_reversalGops.empty()) {
        return 0.0;
    }

    /*double total = 0.0;
    double t1 = 0.0;
    for (auto& it : m_reversalGops) {
        auto t = it->getDuration();
        total += t;
        if (t > 0.0) {
            if (t1 > 0.0) {
                double t2 = abs(t1 - it->m_gop.back()->pts);
                total += t2;
            }
            t1 = it->m_gop.front()->pts;
        }
    }*/

    double total = m_reversalGops.back()->getLastPts() - m_reversalGops.front()->getFirstPts() + 0.04;
    return total;
}

int FrameGops::putMyFrame(MyFrame::Ptr frame) {
    if (!frame) {
        return -1;
    }
    FrameGop::Ptr gop;
    if (m_reversalGops.empty() || frame->key_frame) {
        gop = make_shared<FrameGop>();
        gop->putMyFrame(frame);
        m_reversalGops.emplace_back(gop);
    }
    else {
        gop = m_reversalGops.back();
        gop->putMyFrame(frame);
    }
    ++m_framSize;
    return 0;
}
