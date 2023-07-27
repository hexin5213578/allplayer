#include <string.h>
#include "mk_rtsp_service.h"
#include "mk_rtsp_connection.h"
#include "mk_rtmp_connection.h"
#include "mk_media_common.h"

mk_rtsp_service::mk_rtsp_service()
{
    m_usUdpStartPort      = RTP_RTCP_START_PORT;
    m_ulUdpPairCount      = RTP_RTCP_PORT_COUNT;
    m_pUdpRtpArray        = NULL;
    m_pUdpRtcpArray       = NULL;
}

mk_rtsp_service::~mk_rtsp_service()
{

}

int32_t mk_rtsp_service::init(uint32_t MaxClient,uint32_t RtpBufCountPerClient)
{
    int32_t nRet = AS_ERROR_CODE_OK;

    nRet = m_msgCache.initBlocks(MaxClient * RtpBufCountPerClient);
    if (AS_ERROR_CODE_OK != nRet)
    {
        MK_LOG(AS_LOG_WARNING,"create rtp recv bufs fail.");
        return nRet;
    }

    MK_LOG(AS_LOG_INFO,"create rtp recv bufs success.");
    
    nRet = create_rtp_rtcp_udp_pairs();
    if (AS_ERROR_CODE_OK != nRet)
    {
        MK_LOG(AS_LOG_WARNING,"create rtp and rtcp pairs fail.");
        return nRet;
    }

    return AS_ERROR_CODE_OK;
}

void mk_rtsp_service::release()
{
    m_msgCache.release();
    destory_rtp_rtcp_udp_pairs();
    return;
}
void    mk_rtsp_service::set_rtp_rtcp_udp_port(uint16_t udpPort,uint32_t count)
{
    m_usUdpStartPort      = udpPort;
    m_ulUdpPairCount      = count;
}

void    mk_rtsp_service::get_rtp_rtcp_udp_port(uint16_t& udpPort,uint32_t& count)
{
    udpPort = m_usUdpStartPort;
    count   = m_ulUdpPairCount;
}

int32_t mk_rtsp_service::get_rtp_rtcp_pair(mk_rtsp_udp_handle*& pRtpHandle,mk_rtsp_udp_handle*& pRtcpHandle)
{
    if(0 == m_RtpRtcpfreeList.size()) 
        return AS_ERROR_CODE_FAIL;

    uint32_t index = m_RtpRtcpfreeList.front();
    m_RtpRtcpfreeList.pop_front();
    pRtpHandle  = m_pUdpRtpArray[index];
    pRtcpHandle = m_pUdpRtcpArray[index];
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_service::free_rtp_rtcp_pair(mk_rtsp_udp_handle* pRtpHandle)
{
    uint32_t index = pRtpHandle->get_index();
    m_RtpRtcpfreeList.push_back(index);
    return;
}

as_msg_block* mk_rtsp_service::get_rtp_recv_buf(uint32_t buffSize)
{
    return m_msgCache.getMsgBlock(buffSize);
}

void mk_rtsp_service::free_rtp_recv_buf(as_msg_block* block)
{
    return m_msgCache.freeMsgBlock(block);
}

int32_t mk_rtsp_service::create_rtp_rtcp_udp_pairs()
{
    MK_LOG(AS_LOG_INFO,"create udp rtp and rtcp pairs,start port:[%d],count:[%d].",m_usUdpStartPort,m_ulUdpPairCount);
    
    uint32_t port = m_usUdpStartPort;
    uint32_t pairs = m_ulUdpPairCount/2;
    mk_rtsp_udp_handle*  pRtpHandle  = NULL;
    mk_rtsp_udp_handle* pRtcpHandle = NULL;
    int32_t nRet = AS_ERROR_CODE_OK;

    m_pUdpRtpArray  = AS_NEW(m_pUdpRtpArray,pairs);
    if(NULL == m_pUdpRtpArray)
    {
        MK_LOG(AS_LOG_CRITICAL,"create rtp handle array fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_pUdpRtcpArray = AS_NEW(m_pUdpRtcpArray,pairs);
    if(NULL == m_pUdpRtcpArray) 
    {
        MK_LOG(AS_LOG_CRITICAL,"create rtcp handle array fail.");
        return AS_ERROR_CODE_FAIL;
    }

    for(uint32_t i = 0;i < pairs;i++)
    {
        pRtpHandle = AS_NEW(pRtpHandle);
        if(NULL == pRtpHandle) 
        {
            MK_LOG(AS_LOG_CRITICAL,"create rtp handle object fail.");
            return AS_ERROR_CODE_FAIL;
        }        
        pRtpHandle->init(i,port);
        port++;

        pRtcpHandle = AS_NEW(pRtcpHandle);
        if(NULL == pRtcpHandle) 
        {
            MK_LOG(AS_LOG_CRITICAL,"create rtcp handle object fail.");
            return AS_ERROR_CODE_FAIL;
        }        
        pRtcpHandle->init(i,port);
        port++;
        m_pUdpRtpArray[i]  = pRtpHandle;
        m_pUdpRtcpArray[i] = pRtcpHandle;
        m_RtpRtcpfreeList.push_back(i);
    }
    MK_LOG(AS_LOG_INFO,"create udp rtp and rtcp pairs success.");
    return AS_ERROR_CODE_OK;
}
void    mk_rtsp_service::destory_rtp_rtcp_udp_pairs()
{
    MK_LOG(AS_LOG_INFO,"m_pUdpRtcpArray udp rtp and rtcp pair.");

    mk_rtsp_udp_handle*  pRtpHandle  = NULL;
    mk_rtsp_udp_handle* pRtcpHandle = NULL;

    for(uint32_t i = 0; i < m_ulUdpPairCount/2; i++)
    {
        pRtpHandle  = m_pUdpRtpArray[i];
        pRtcpHandle = m_pUdpRtcpArray[i];

        if(NULL != pRtpHandle)
        {
			pRtpHandle->stop_handle();
            AS_DELETE(pRtpHandle);
            m_pUdpRtpArray[i] = NULL;
        }
  
        if(NULL != pRtcpHandle) 
        {
			pRtcpHandle->stop_handle();
            AS_DELETE(pRtcpHandle);
            m_pUdpRtcpArray[i] = NULL;
        }
    }
    MK_LOG(AS_LOG_INFO,"destory udp rtp and rtcp pairs success.");
    return;
}
