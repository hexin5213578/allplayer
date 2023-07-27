#include "step_deque.h"
#include "as_log.h"
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/rational.h>
}

const int STEP_FRAME_SIZE = 1024;
const int MAX_STEP_BUFF_SIZE = 1025;

StepDeque::StepDeque()
{
   
}

void StepDeque::setStepCapcity(int step_size)
{
    m_step_capcity = step_size;
    m_status = (0 == m_step_capcity) ? REALTIME : BUFFERING;
}

int StepDeque::getQueueSize()
{
    return m_pkts_queue.size();
}

int StepDeque::getCacheSize()
{
    std::unique_lock<std::mutex> lck(m_listMtx);
    if (BUFFERING == m_status) {
        return m_pkts_queue.size() - m_cur_index;
    }
    return 0;
}

bool StepDeque::stepSatify(int8_t stepDirect)
{
    if (REALTIME == m_status) {
        return false;
    }
    else if (BUFFERING == m_status) {
        m_status = StepDeque::STEPPING;
        return true;
    }

    //其他情况通过缓冲空或满判断
    if ((stepDirect > 0 && m_cur_index >= MAX_STEP_BUFF_SIZE) || (stepDirect < 0 && m_cur_index <= 0)) {
        return false;
    }
    return true;
}

int StepDeque::forward(AVPacket** pkt)
{
    *pkt = nullptr;
    std::unique_lock<std::mutex> lck(m_listMtx);
    
    if (m_cur_index >= MAX_STEP_BUFF_SIZE - 1) {
        return ERR_OUTRANG;
    }

    if (0 == m_forward_size) {
        return ERR_AGAIN;
    }
    
    AS_LOG(AS_LOG_DEBUG, "forward --- m_cur_index:%d, m_pkts_queue siz:%d", m_cur_index, m_pkts_queue.size());
    int ret = ERR_AGAIN;
    int tempIdx = m_cur_index;
    if(tempIdx >= 0 && tempIdx < m_pkts_queue.size()) {
        auto packet = m_pkts_queue[tempIdx];
        *pkt = av_packet_alloc();
        av_packet_ref(*pkt, packet);
        --m_forward_size;
        ++m_backward_size;
        m_cur_index = ++tempIdx;
        ret = NOERR;
    }
   
    //shrink缓存区
    while (m_backward_size > m_step_capcity * 1.5 && m_cur_index > 0) {
        auto frontPkt = m_pkts_queue.front();
        av_packet_free(&frontPkt);
        m_pkts_queue.pop_front();
        --m_backward_size;
        --m_cur_index;
    }
    return ret;
}

int StepDeque::backward(AVPacket** pkt)
{
    AS_LOG(AS_LOG_DEBUG, "backward --- m_cur_index:%d, m_pkts_queue siz:%d", m_cur_index, m_pkts_queue.size());
    *pkt = nullptr;
    int tempIndex = m_cur_index;
    std::unique_lock<std::mutex> lck(m_listMtx);
    if (tempIndex >= 0 && tempIndex < m_pkts_queue.size()) {
        while (--tempIndex >= 0) {
            auto packet = m_pkts_queue[tempIndex];
            if (packet->flags & AV_PKT_FLAG_KEY) {
                *pkt = av_packet_alloc();
                av_packet_ref(*pkt, packet);
                m_forward_size += 1;
                m_backward_size -= 1;
                m_cur_index = --tempIndex;
                return NOERR;
            }
        }
    }
    return ERR_OUTRANG;
}

int StepDeque::push(AVPacket* pkt)
{
    auto packet = av_packet_alloc();
    //引用计数 减少数据复制，线程安全
    av_packet_ref(packet, pkt);
    //FixBug:内存泄露
    av_packet_unref(pkt);
    std::unique_lock<std::mutex> lck(m_listMtx);
    m_pkts_queue.push_back(packet);
    ++m_pktSize;
    int ret = NOERR;

    switch (m_status) {
    case StepDeque::REALTIME:
        if (m_pktSize > MAX_STEP_BUFF_SIZE + m_step_capcity) {      //clear
            AS_LOG(AS_LOG_WARNING, "video pkts size is reach to max, %d, clear until keyframe", m_pktSize);
            
            auto frontPkt = m_pkts_queue.front();
            if (frontPkt->flags & AV_PKT_FLAG_KEY) {    //front is key frame, clear
                --m_pktSize;
                av_packet_free(&frontPkt);
                m_pkts_queue.pop_front();
            }
     
            while (!m_pkts_queue.empty()) {         //clear until key frame
                frontPkt = m_pkts_queue.front();
                if (frontPkt->flags & AV_PKT_FLAG_KEY) {
                    return ERR_CLRAR;;
                }
                --m_pktSize;
                av_packet_free(&frontPkt);
                m_pkts_queue.pop_front();
            }
            return ERR_CLRAR;
        }
        break;
    case StepDeque::BUFFERING:
        ++m_backward_size;
        //AS_LOG(AS_LOG_INFO, "push packet to step buffer, %d video can be stepped !", m_backward_size);
        break;
    case StepDeque::STEPPING:
        ++m_forward_size;
        ret = m_forward_size;
        break;
    case StepDeque::WINDINGUP:
        AS_LOG(AS_LOG_WARNING, "push packet to step buffer when winding up,be careful !");
        ++m_forward_size;
        break;
    default:
        break;
    }
    return ret;
}

int StepDeque::pop(AVPacket** pkt)
{
    *pkt = nullptr;
    std::unique_lock<std::mutex> lock(m_listMtx);

    if (m_pkts_queue.empty()) {
        return ERR_AGAIN;
    }

    AVPacket* frontPkt = nullptr;
    switch (m_status) {
    case StepDeque::REALTIME:
        *pkt = av_packet_alloc();
        frontPkt = m_pkts_queue.front();
        av_packet_ref(*pkt, frontPkt);
        av_packet_free(&frontPkt);
        m_pkts_queue.pop_front();
        --m_pktSize;
        return NOERR;
        break;
    case StepDeque::BUFFERING:
        if (m_backward_size > m_step_capcity) {      //清理前向缓冲至指定大小
            while (m_backward_size > m_step_capcity && m_cur_index > 0) {
                frontPkt = m_pkts_queue.front();
                av_packet_free(&frontPkt);
                m_pkts_queue.pop_front();
                --m_backward_size;
                --m_cur_index;
            }
        }
        
        if (m_cur_index >= m_pkts_queue.size()) {
            return ERR_AGAIN;
        }
        else if (m_cur_index < m_pkts_queue.size()) {
            *pkt = av_packet_alloc();
            av_packet_ref(*pkt, m_pkts_queue[m_cur_index]);
        }
        ++m_cur_index;
        return NOERR;
        break;
    case StepDeque::STEPPING:
        AS_LOG(AS_LOG_WARNING, "pop packet when stepping !");
        break;
    case StepDeque::WINDINGUP:
        if (m_cur_index < m_pkts_queue.size()) {
            *pkt = av_packet_alloc();
            av_packet_ref(*pkt, m_pkts_queue[m_cur_index++]);
            return NOERR;
        }

        while (m_backward_size > m_step_capcity && m_cur_index >0) {    //清理完毕，重置状态
            auto frontPkt = m_pkts_queue.front();
            av_packet_free(&frontPkt);
            m_pkts_queue.pop_front();
            --m_backward_size;
            --m_cur_index;
        }
        m_forward_size = 0;
        m_status = BUFFERING;
        return ERR_WINDEND;
        break;
    default:
        break;
    }
    return ERR_OUTRANG;
}

void StepDeque::stopStep()
{
    m_status = WINDINGUP;
    std::unique_lock<std::mutex> lck(m_listMtx);
    while (m_cur_index > m_step_capcity) {
        auto frontPkt = m_pkts_queue.front();
        av_packet_free(&frontPkt);
        m_pkts_queue.pop_front();
        --m_backward_size;
        --m_cur_index;
    }
}

StepDeque::STATUS StepDeque::getDequeStatus()
{
    return m_status;
}

bool StepDeque::checkLevel()
{
    return false;
}

void StepDeque::clear()
{
    m_backward_size = 0;
    m_forward_size = 0;
    m_cur_index = 0;
    std::unique_lock<std::mutex> lck(m_listMtx);
    for (auto& pkt : m_pkts_queue) {
        av_packet_free(&pkt);
    }
    m_pkts_queue.clear();
    if (REALTIME != m_status) {
        m_status = BUFFERING;
    }
}
