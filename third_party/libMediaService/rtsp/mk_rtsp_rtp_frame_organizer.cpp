#include "as.h"
#include "mk_rtsp_rtp_frame_organizer.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_rtsp_service.h"
#include "mk_media_common.h"

mk_rtp_frame_organizer::mk_rtp_frame_organizer()
{
    m_bCheckStrict          = false;
    m_unMaxCacheFrameNum    = 0;
    m_pRtpFrameHandler      = NULL;
    m_bFirstRtpPacket       = true;
    m_unTotalRtpPacketNum   = 0;
    m_unLostRtpPacketNum    = 0;
    m_unLostFrameNum        = 0;
    m_unDisorderSeqCounts   = 0;
    m_unLastRtpSeq          = UINT32_MAX;
}

mk_rtp_frame_organizer::~mk_rtp_frame_organizer()
{
    try
    {
        release();
    }
    catch(...){}

    m_pRtpFrameHandler = NULL;
}

int32_t mk_rtp_frame_organizer::init(mk_rtp_frame_handler* pHandler, bool strict, uint32_t unMaxFrameCache /*= MAX_RTP_FRAME_CACHE_NUM*/)
{
    if ((NULL == pHandler) || (0 == unMaxFrameCache)) {
        return AS_ERROR_CODE_FAIL;
    }
    m_bCheckStrict = strict;
    m_pRtpFrameHandler   = pHandler;
    m_unMaxCacheFrameNum = unMaxFrameCache;

    RTP_FRAME_INFO_S *pFramInfo = NULL;

    for(uint32_t i = 0;i < m_unMaxCacheFrameNum;i++) {
        pFramInfo = AS_NEW(pFramInfo);
        if(NULL == pFramInfo) {
            return AS_ERROR_CODE_FAIL;
        }
        m_RtpFrameFreeList.push_back(pFramInfo);
    }

    MK_LOG(AS_LOG_INFO,"success to init rtp frame organizer[%p].", this);
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_frame_organizer::insertRtpPacket(as_msg_block* pRtpBlock)
{
    if (!pRtpBlock) {
        return AS_ERROR_CODE_FAIL;
    }
      
    mk_rtp_packet rtpPacket;
    if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pRtpBlock->base(), pRtpBlock->length())) {
        MK_LOG(AS_LOG_WARNING, "fail to insert rtp packet, parse rtp packet fail.");
        return AS_ERROR_CODE_FAIL;
    }

    RTP_FRAME_INFO_S* pFrameInfo = NULL;

    RTP_PACK_INFO_S  rtpInfo;
    rtpInfo.bMarker      = rtpPacket.GetMarker();
    rtpInfo.usSeq        = rtpPacket.GetSeqNum();
    rtpInfo.unTimestamp  = rtpPacket.GetTimeStamp();
    rtpInfo.pRtpMsgBlock = pRtpBlock;

    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.find(rtpInfo.unTimestamp);
    if(iter == m_RtpFrameMap.end()) {
        pFrameInfo = insertFrame(rtpPacket.GetPayloadType(),rtpInfo.unTimestamp);
    }
    else {
        pFrameInfo = iter->second;
    }

    if(!pFrameInfo) { 
        //avoid lost packets frequent
        MK_LOG(AS_LOG_INFO,"insert frame fail,becaus there is no free frame info.");   
        RTP_FRAME_INFO_S *smallFrame = getSmallFrame();
       
        if (m_bCheckStrict) {  //处理过期数据
            uint16_t lostPktNb = 0;
            uint16_t usCount = smallFrame->packetQueue.size();

            uint16_t usMaxSeq = smallFrame->packetQueue.back().usSeq;
            uint16_t usMinSeq = smallFrame->packetQueue.front().usSeq;
            if (usMinSeq > usMaxSeq) {
                lostPktNb = usCount;
            }
            else {
                uint16_t usSeqLen = usMaxSeq - usMinSeq + 1;
                if (usSeqLen > usCount) {
                    lostPktNb = (usSeqLen - usCount);
                }
            }
            handleRtpLostPkt(lostPktNb);
            releaseRtpPacket(smallFrame);
        }
        else {
            smallFrame->bMarker = true;
            processFrame();           //process the oldest frame
        }
        pFrameInfo = insertFrame(rtpPacket.GetPayloadType(), rtpInfo.unTimestamp);  // reinsert
    }

    if(pFrameInfo && false == pFrameInfo->bMarker) {
        pFrameInfo->bMarker = rtpInfo.bMarker;
    }

    if (AS_ERROR_CODE_OK != insert(pFrameInfo, rtpInfo)) {
        return AS_ERROR_CODE_FAIL;
    }
    checkFrame();
    return AS_ERROR_CODE_OK;
}

void mk_rtp_frame_organizer::release()
{
    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.begin();
    RTP_FRAME_INFO_S *pFramInfo = NULL;
    for(;iter != m_RtpFrameMap.end();++iter)
    {
        pFramInfo = iter->second;
        if(NULL == pFramInfo)
        {
            continue;
        }
        while (!pFramInfo->packetQueue.empty())
        {
            mk_rtsp_service::instance().free_rtp_recv_buf(pFramInfo->packetQueue.front().pRtpMsgBlock);
            pFramInfo->packetQueue.pop_front();
        }

        AS_DELETE(pFramInfo);
    }

    m_RtpFrameMap.clear();

    while(!m_RtpFrameFreeList.empty())
    {
        pFramInfo = m_RtpFrameFreeList.front();
        m_RtpFrameFreeList.pop_front();
        AS_DELETE(pFramInfo);
    }

    MK_LOG(AS_LOG_INFO,"success to release rtp frame organizer[%p].", this);
    return;
}

void mk_rtp_frame_organizer::getRtpPacketStatInfo(uint32_t &totalPackNum,uint32_t &lostRtpPacketNum,uint32_t &lostFrameNum,uint32_t &disorderSeqCounts)
{
    totalPackNum      = m_unTotalRtpPacketNum;
    lostRtpPacketNum  = m_unLostRtpPacketNum;
    lostFrameNum      = m_unLostFrameNum;
    disorderSeqCounts = m_unDisorderSeqCounts;
    return;
}

void mk_rtp_frame_organizer::updateTotalRtpPacketNum()
{
    if (UINT32_MAX == m_unTotalRtpPacketNum) 
    {
        m_unTotalRtpPacketNum = 0;
        m_unLostRtpPacketNum = 0;
    }
    m_unTotalRtpPacketNum++;
}

int32_t mk_rtp_frame_organizer::insert(RTP_FRAME_INFO_S *pFrameinfo,const RTP_PACK_INFO_S &info)
{
    if(NULL == pFrameinfo) 
        return AS_ERROR_CODE_FAIL;

    if (0 == pFrameinfo->packetQueue.size())
    {
        pFrameinfo->uSeqMax = info.usSeq;
        pFrameinfo->packetQueue.push_back(info);
        return AS_ERROR_CODE_OK;
    }

    if (pFrameinfo->uSeqMax < info.usSeq) 
        pFrameinfo->uSeqMax = info.usSeq;

    if (UINT16_MAX == pFrameinfo->uSeqMax) {
        MK_LOG(AS_LOG_DEBUG, "%d flip when arrive at UINT16_MAX.", info.usSeq, UINT16_MAX);
        pFrameinfo->packetQueue.push_back(info);
        return AS_ERROR_CODE_OK;
    }

    uint16_t usFirstSeq = pFrameinfo->packetQueue.front().usSeq;
    uint16_t usLastSeq = pFrameinfo->packetQueue.back().usSeq;
    
    if (info.usSeq >= usLastSeq)
    {
        pFrameinfo->packetQueue.push_back(info);
    }
    else if(info.usSeq < usFirstSeq)
    {
        pFrameinfo->packetQueue.push_front(info);
    }
    else
    {
        return insertRange(pFrameinfo,info);
    }
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_frame_organizer::insertRange(RTP_FRAME_INFO_S *pFrameinfo ,const RTP_PACK_INFO_S &info)
{
    uint32_t i = 0;
    uint32_t unSize = pFrameinfo->packetQueue.size();
    while (i <= unSize)
    {
        if (info.usSeq <= pFrameinfo->packetQueue[i].usSeq)
        {
            pFrameinfo->packetQueue.insert(pFrameinfo->packetQueue.begin() + (int32_t)i, info);
            return AS_ERROR_CODE_OK;
        }
        i++;
    }

    MK_LOG(AS_LOG_WARNING,"fail to insert rtp packet[%u : %u].",info.usSeq, info.unTimestamp);
    return AS_ERROR_CODE_FAIL;
}

void mk_rtp_frame_organizer::checkFrame()
{
    RTP_FRAME_INFO_S *pFrameinfo = getSmallFrame();
    if(NULL == pFrameinfo) {
        MK_LOG(AS_LOG_DEBUG,"get small frame fail.");
        return;
    }
    if (pFrameinfo->packetQueue.empty()) {
        MK_LOG(AS_LOG_INFO,"small frame is empty.");
        return;
    }

    if (!pFrameinfo->bMarker) {
        return;
    }

    uint16_t usSeq = (uint16_t)pFrameinfo->packetQueue.front().usSeq;
    int unmatched = 0;
    for (uint32_t i = 0; i < pFrameinfo->packetQueue.size(); ) {
		uint16_t usSeqTmp = pFrameinfo->packetQueue[i].usSeq;
        //如果发生跳帧，usSeqTmp不变，则只递增unSeq进行下次匹配 
        if (usSeq != usSeqTmp) {    
            if (++unmatched <= 3) {
                ++usSeq;
                continue;
            }
            else {
                MK_LOG(AS_LOG_INFO, "seq umatched exceed more than 3 times, drop.");
                return;
            }
        }
        i++;
        usSeq++;
    }
   
    if (unmatched > 0) {
        MK_LOG(AS_LOG_INFO, "contians seq umatched rtp, times: %d.", unmatched);
    }

    handleFinishedFrame(pFrameinfo);
    releaseRtpPacket(pFrameinfo);
    return;
}

void mk_rtp_frame_organizer::processFrame()
{
    RTP_FRAME_INFO_S* pFrameinfo = getSmallFrame();

    if (NULL == pFrameinfo) 
    {
        MK_LOG(AS_LOG_DEBUG, "Get Small Frame fail.");
        return;
    }

    if (!pFrameinfo->bMarker) {
        return;
    }

    uint16_t lostPktNb = 0;
    uint16_t usCount = pFrameinfo->packetQueue.size();
    uint16_t usMaxSeq = pFrameinfo->packetQueue.back().usSeq;
    uint16_t usMinSeq = pFrameinfo->packetQueue.front().usSeq;
    
    if(usMinSeq < usMaxSeq) 
    {
        uint16_t usSeqLen = usMaxSeq - usMinSeq + 1;
        if (usSeqLen > usCount) {
            lostPktNb = usSeqLen - usCount;
        }
    }
    if (lostPktNb) 
    {
        handleRtpLostPkt(lostPktNb);
    }

    handleFinishedFrame(pFrameinfo);
    releaseRtpPacket(pFrameinfo);
    return;
}

void mk_rtp_frame_organizer::handleRtpLostPkt(uint16_t usLostPacket)
{
    m_unLostFrameNum++;
    m_unLostRtpPacketNum += usLostPacket;
}

void mk_rtp_frame_organizer::handleFinishedFrame(RTP_FRAME_INFO_S *pFrameinfo)
{
    if(NULL == pFrameinfo) 
    {
        MK_LOG(AS_LOG_WARNING," handle finish frame ,the frame info is NULL");
        return;
    }

    if (pFrameinfo->packetQueue.empty()) 
    {
        MK_LOG(AS_LOG_WARNING," handle finish frame ,the frame queue is empty");
        return;
    }

    if (NULL == m_pRtpFrameHandler) 
    {
        MK_LOG(AS_LOG_WARNING," handle finish frame ,the rtp handler is NULL");
        return;
    }

    m_pRtpFrameHandler->handleRtpFrame(pFrameinfo->payloadType,pFrameinfo->packetQueue);
    return;
}

void mk_rtp_frame_organizer::releaseRtpPacket(RTP_FRAME_INFO_S *pFrameinfo)
{
    if(NULL == pFrameinfo) 
    {
        return;
    }

    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.find(pFrameinfo->unTimestamp);
    if(iter == m_RtpFrameMap.end())
    {
        return;
    }

    while (!pFrameinfo->packetQueue.empty())
    {
        if (NULL != pFrameinfo->packetQueue.front().pRtpMsgBlock)
        {
            mk_rtsp_service::instance().free_rtp_recv_buf(pFrameinfo->packetQueue.front().pRtpMsgBlock);
        }
        pFrameinfo->packetQueue.pop_front();
    }
    pFrameinfo->packetQueue.clear();

    m_RtpFrameMap.erase(iter);
    m_RtpFrameFreeList.push_back(pFrameinfo);
    return;
}

RTP_FRAME_INFO_S* mk_rtp_frame_organizer::insertFrame(uint8_t PayloadType, uint32_t unTimestamp)
{
    if (0 == m_RtpFrameFreeList.size())
        return NULL;
       
    RTP_FRAME_INFO_S* pFrame = m_RtpFrameFreeList.front();
    m_RtpFrameFreeList.pop_front();
    pFrame->bMarker = false;
    pFrame->payloadType = PayloadType;
    pFrame->unTimestamp = unTimestamp;
    pFrame->uSeqMax = 0;
    m_RtpFrameMap.insert(RTP_FRAME_MAP_S::value_type(unTimestamp,pFrame));
    return pFrame;
}

RTP_FRAME_INFO_S* mk_rtp_frame_organizer::getSmallFrame()
{
    RTP_FRAME_MAP_S::iterator iter = m_RtpFrameMap.begin();
    RTP_FRAME_INFO_S *pCurFramInfo = NULL;
    RTP_FRAME_INFO_S *pSmallFramInfo = NULL;
    for(;iter != m_RtpFrameMap.end();++iter)
    {
        pCurFramInfo = iter->second;
        if(NULL == pCurFramInfo)
        {
            continue;
        }
        if(NULL == pSmallFramInfo)
        {
            pSmallFramInfo = pCurFramInfo;
            continue;
        }

        if(pSmallFramInfo->unTimestamp >  pCurFramInfo->unTimestamp)
        {
            pSmallFramInfo = pCurFramInfo;
        }
    }
    return pSmallFramInfo;
}

