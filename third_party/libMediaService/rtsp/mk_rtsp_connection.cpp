
#include <sstream>
#include "mk_rtsp_connection.h"
#include "mk_rtsp_packet.h"
#include "mk_media_service.h"
#include "mk_rtsp_service.h"
#include "mk_media_common.h"
#include "mk_rtsp_rtp_packet.h"
#include "as_socks5_util.h"

#if ((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include "aac.h"
#include "h264.h"
#include "hevc.h"
#include "mjpeg.h"
#include "g711.h"
#else
#include "extend/as_frame.h"
#include "track/aac.h"
#include "track/h264.h"
#include "track/hevc.h"
#include "track/g711.h"
#include "track/mjpeg.h"
#endif

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <arpa/inet.h>
#include <sys/socket.h>
#elif AS_APP_OS == AS_OS_WIN32
#include <Ws2tcpip.h>
#endif

#define RSTP_SETUP_CHANNELS_TIMEOUT     30
#define KB_SIZE                         1024

mk_rtsp_connection::mk_rtsp_connection()
{
    m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]    = NULL;
    m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]   = NULL;
    m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]    = NULL;
    m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]   = NULL;

    as_init_url(&m_url);
    as_init_url(&m_sockUrl);
    m_url.port        = RTSP_DEFAULT_PORT;
    m_sockUrl.port    = SOCKS5_DEFAULT_PORT;
    m_ulRecvSize      = 0;
    m_ulSeq           = 0;
    m_bSocks          = false;
    m_Status          = RTSP_SESSION_STATUS_INIT;
    m_bSetupTcp       = false;
    m_bSetupSuccess   = false;
    if(NULL != m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]) 
    {
        mk_rtsp_service::instance().free_rtp_rtcp_pair(m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]);
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]    = NULL;
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]   = NULL;
    }

    if(NULL != m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]) 
    {
        mk_rtsp_service::instance().free_rtp_rtcp_pair(m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]);
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]    = NULL;
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]   = NULL;
    }

    m_ulLastRecv = time(NULL);
    m_ulLastRtcpSend = time(NULL);
    m_ulRtcpStartTime = m_ulLastRecv;
    m_ulAuthenTime = 0;
    m_strAuthenticate = "";
    m_strSdpInfo = "";
	m_strConBase = "";
	m_sessionId  = "";
    m_fragmentIndex = 0;
    m_fps = 0;
    m_unDropFps = 0;
    m_ulRecvByteSize = 0;
    m_ulStreamSizeKB = 0;
    m_recvState = kNormal;
    m_bVoiceTalk = false;

    if (m_pFormatCtx = AS_NEW(m_pFormatCtx)) 
    {
        //memset(m_pFormatCtx, 0, sizeof(m_pFormatCtx));
        m_sdpInfo.setFormatContex(m_pFormatCtx);
    }
}

mk_rtsp_connection::~mk_rtsp_connection()
{
    if(NULL != m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]) 
    {
        mk_rtsp_service::instance().free_rtp_rtcp_pair(m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]);
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]    = NULL;
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]   = NULL;
    }

    if(NULL != m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]) 
    {
        mk_rtsp_service::instance().free_rtp_rtcp_pair(m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]);
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]    = NULL;
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]   = NULL;
    }

    if (m_pFormatCtx) 
    {
        m_pFormatCtx->release();
        AS_DELETE(m_pFormatCtx);
    }
}

int32_t mk_rtsp_connection::setupConnection()
{
    if (AS_ERROR_CODE_OK != as_digest_init(&m_Authen, 1)) 
        return AS_ERROR_CODE_FAIL;

    MK_LOG(AS_LOG_INFO, "rtsp client connect url [%s] ", m_strurl.c_str());
	if (AS_ERROR_CODE_OK != as_parse_url(m_strurl.c_str(), &m_url)) {
		AS_LOG(AS_LOG_ERROR, "parse url faild.");
		return AS_ERROR_CODE_FAIL;
	}

    as_network_svr* pNetWork = mk_media_service::instance().get_client_network_svr(this->get_index());
    
    if (m_bSocks) 
    {
        as_network_addr sock_local;
        as_network_addr sock_remote;

        if (AS_ERROR_CODE_OK != getAddrInfo(&sock_remote.m_ailist))
            return AS_ERROR_CODE_FAIL;

        struct in_addr sin_addr;    /* AF_INET */
        struct in6_addr sin_addr6;    /* AF_INET6 */

        sock_remote.m_sa_family = sock_remote.m_ailist->ai_family;

        if (sock_remote.m_ailist->ai_family == AF_INET)
        {
            inet_pton(AF_INET, (char*)&m_sockUrl.host[0], &sin_addr);
            sock_remote.m_ulIpAddr = sin_addr.s_addr;
        }
        else if (sock_remote.m_ailist->ai_family == AF_INET6)
        {
			inet_pton(AF_INET6, (char*)&m_sockUrl.host[0], &sin_addr6);
			memcpy(&sock_remote.m_strAddr, &m_sockUrl.host, NETWORK_ADDR_STR_LEN);
        }

        sock_remote.m_usPort = htons(m_sockUrl.port);

        if (AS_ERROR_CODE_OK != pNetWork->regTcpClient(&sock_local, &sock_remote, this, enSyncOp, MK_CONNECT_TIMEOUT)) 
        {
            handle_connection_status(MR_CLIENT_CONNECT_FAILED);
            return AS_ERROR_CODE_FAIL;
        }
        
        int32_t ret = socks5Greeting();
        if (ret)
        {
            handle_connection_status(MR_CLIENT_CONNECT_FAILED);
            return AS_ERROR_CODE_FAIL;
        }
        m_Status = RTSP_SOCK5_PROXY_HANDSHAKE;
    }
    else 
    {
        as_network_addr local;
        as_network_addr remote;

        if (AS_ERROR_CODE_OK != getAddrInfo(&remote.m_ailist))
            return AS_ERROR_CODE_FAIL;

        struct in_addr sin_addr;    /* AF_INET */
        struct in6_addr sin_addr6;    /* AF_INET6 */

        remote.m_sa_family = remote.m_ailist->ai_family;

		if (remote.m_ailist->ai_family == AF_INET)
		{
			inet_pton(AF_INET, (char*)&m_url.host[0], &sin_addr);
            remote.m_ulIpAddr = sin_addr.s_addr;
		}
		else if (remote.m_ailist->ai_family == AF_INET6)
		{
			inet_pton(AF_INET6, (char*)&m_url.host[0], &sin_addr6);
			memcpy(&remote.m_strAddr, &m_url.host, NETWORK_ADDR_STR_LEN);

		}

        if (0 == m_url.port)
            remote.m_usPort = htons(RTSP_DEFAULT_PORT);
        else 
            remote.m_usPort = htons(m_url.port);

        MK_LOG(AS_LOG_INFO, "rtsp client connect to [%s:%d] ,uri:[%s][%d],args[%s]",
            (char*)&m_url.host[0], m_url.port, (char*)&m_url.uri[0], AS_URL_MAX_LEN, (char*)&m_url.args[0]);

        if (AS_ERROR_CODE_OK != pNetWork->regTcpClient(&local, &remote, this, enSyncOp, MK_CONNECT_TIMEOUT)) 
        {
            handle_connection_status(MR_CLIENT_CONNECT_FAILED);
            return AS_ERROR_CODE_FAIL;
        }
        handle_connection_status(MR_CLIENT_STATUS_CONNECTED
            , getCurConnectPort(remote.m_ailist->ai_addr, remote.m_ailist->ai_addrlen, remote.m_ailist->ai_family));
    }
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_connection::start()
{
    int32_t nRet = setupConnection();
    if (AS_ERROR_CODE_OK != nRet)
        return nRet;

    if(!m_bSocks && AS_ERROR_CODE_OK != this->sendRtspOptionsReq()) 
    {
        MK_LOG(AS_LOG_WARNING,"options:rtsp client send message fail.");
        return AS_ERROR_CODE_FAIL;
    }
    setHandleRecv(AS_TRUE);
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_connection::stop()
{
    sendRtspTeardownReq();

    //resetRtspConnect();
    setHandleRecv(AS_FALSE);
    /* unregister the network service */
    as_network_svr* pNetWork = mk_media_service::instance().get_client_network_svr(this->get_index());
    pNetWork->removeTcpClient(this);

    if (NULL != m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE])
    {
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]->stop_handle();
    }
    if (NULL != m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]) 
    {
        m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]->stop_handle();
    }

    if (NULL != m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]) 
    {
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]->stop_handle();
    }
    if (NULL != m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]) 
    {
        m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]->stop_handle();
    }

    if (m_pFormatCtx)
    {
        m_pFormatCtx->release();
        AS_DELETE(m_pFormatCtx);
    }

    MK_LOG(AS_LOG_INFO, "avs close rtsp client.");
    return;
}

void mk_rtsp_connection::set_socks5(const char* socks_addr, uint16_t port, const char* user, const char* pass)
{
    m_bSocks = true;
    strncpy(m_sockUrl.host, socks_addr, strlen(socks_addr));
    m_sockUrl.port = port;
    if (user) 
        strncpy(m_sockUrl.username, user, strlen(user));
    
    if (pass) 
        strncpy(m_sockUrl.password, pass, strlen(pass));

    if (user || pass) 
        m_bAuth = true;
    else 
        m_bAuth = false;
}

void mk_rtsp_connection::pause()
{
	if (AS_ERROR_CODE_OK != this->sendRtspPauseReq()) 
    {
		MK_LOG(AS_LOG_WARNING, "pause:rtsp client send message fail.");
		return;
	}
}

void mk_rtsp_connection::vcr_control(double start,double scale, double speed)
{
	if (AS_ERROR_CODE_OK != this->sendRtspPlayReq(start, scale, speed)) 
    {
		MK_LOG(AS_LOG_WARNING, "speed:rtsp client send message fail. [%lld] [%f]", start,speed);
		return;
	}
}

int32_t mk_rtsp_connection::recv_next()
{
    m_bDoNextRecv = AS_TRUE;
    return AS_ERROR_CODE_OK;
}

void  mk_rtsp_connection::check_client()
{
    time_t cur = time(NULL);

    if (!m_bSetupSuccess) {
        if (RSTP_SETUP_CHANNELS_TIMEOUT < (cur - m_ulRtcpStartTime)) {
            handle_connection_status(MR_CLIENT_SETUP_TIMEOUT);
            m_ulRtcpStartTime = cur;
        }
    }   
    else {
        if (MK_CLIENT_RECV_TIMEOUT < (cur - m_ulLastRecv)) {
            handle_connection_status(MR_CLIENT_STATUS_TIMEOUT);
            m_ulLastRecv = cur;
        }
    }
    return;
}

void  mk_rtsp_connection::set_rtp_over_tcp()
{
    m_bSetupTcp = true;
    return;
}

void mk_rtsp_connection::set_vcr_parameter(VcrControllSt& vcrst)
{
    m_vcrSt = vcrst;
    return;
}

void  mk_rtsp_connection::get_rtp_stat_info(RTP_PACKET_STAT_INFO* statinfo)
{
    statinfo->ulStreamSizeKb = m_ulStreamSizeKB * 8;
    statinfo->dlTraffic = (double)m_ulStreamSizeKB + (double)m_ulRecvByteSize / KB_SIZE;

    if (m_pFormatCtx) {
        for (int i = 0; i < m_pFormatCtx->streams.size(); ++i) {
            auto st = m_pFormatCtx->streams[i];
            if (st && st->track) {
                st->track->appendRtpStatistics(*statinfo);
            }
        }
    }
    return;
}

void  mk_rtsp_connection::get_rtsp_sdp_info(char* info,uint32_t lens,uint32_t& copylen)
{
    if(m_strSdpInfo.length() > lens)
    {
        copylen = 0;
    }
    else
    {
        memcpy(info,m_strSdpInfo.c_str(),m_strSdpInfo.length());
        copylen = m_strSdpInfo.length();
    }
    return;
}

//#define _DEBUG_AVS

void mk_rtsp_connection::handle_recv(void)
{
    as_network_addr peer;
    int32_t iRecvLen = (int32_t) (MAX_BYTES_PER_RECEIVE -  m_ulRecvSize);
    if (iRecvLen <= 0) {
        MK_LOG(AS_LOG_INFO,"rtsp connection,recv buffer is full, size[%u] length[%u].", MAX_BYTES_PER_RECEIVE, m_ulRecvSize);
        return;
    }

    iRecvLen = this->recv((char*)&m_RecvTcpBuf[m_ulRecvSize],&peer,iRecvLen,enAsyncOp);

    #ifdef _DEBUG_AVS
        MK_LOG(AS_LOG_WARNING, "rtsp connection,recv [%d] len data, data = [0x%02x],[0x%02x],[0x%02x],[0x%02x].", iRecvLen, 
            m_RecvTcpBuf[m_ulRecvSize], m_RecvTcpBuf[m_ulRecvSize + 1], m_RecvTcpBuf[m_ulRecvSize + 2], m_RecvTcpBuf[m_ulRecvSize + 3]);
    #endif // _DEBUG_AVS

    if (iRecvLen < 0) {
        if ((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO)) {
            return;
        }
        //stop_recv();
        handle_connection_status(MR_CLIENT_STATUS_RECV_FAIL);
        MK_LOG(AS_LOG_WARNING, "rtsp connection recv data fail, errno(%d): %s.", CONN_ERRNO, strerror(CONN_ERRNO));
        return;
    }

    if (iRecvLen == 0) {
        handle_connection_status(MR_CLIENT_STATUS_CONN_CLOSE);
        MK_LOG(AS_LOG_WARNING, "rtsp connection has been closed.");
        return;
    }
    
    m_ulRecvByteSize += iRecvLen;
    if (m_ulRecvByteSize >= KB_SIZE) {
        m_ulStreamSizeKB += (m_ulRecvByteSize / KB_SIZE);
        m_ulRecvByteSize %= KB_SIZE;
    }
    m_ulRecvSize += iRecvLen;

    uint32_t processedSize = 0;
    uint32_t totalSize = m_ulRecvSize;
    int32_t nSize = 0;
    do
    {
    #ifdef _DEBUG_AVS
        MK_LOG(AS_LOG_WARNING, "processedIndex = %d, data = [0x%02x],[0x%02x],[0x%02x],[0x%02x].", processedSize, m_RecvTcpBuf[processedSize],
            m_RecvTcpBuf[processedSize + 1], m_RecvTcpBuf[processedSize + 2], m_RecvTcpBuf[processedSize + 3]);
    #endif

        nSize = processRecvedMessage((const char*)&m_RecvTcpBuf[processedSize], m_ulRecvSize - processedSize);
        if (nSize < 0)
        {
            MK_LOG(AS_LOG_WARNING,"rtsp connection process recv data contians error status. ");
            MEDIA_STATUS_INFO statusInfo;
            statusInfo.enStatus = MR_CLIENT_STATUS_SRV_ERROR;
            statusInfo.errCode = m_ulStatusCode;
            handle_connection_status(statusInfo);
            //stop_recv();
            return;
        }
        else if (0 == nSize) 
        {
            break;
        }
       
        #ifdef _DEBUG_AVS
            MK_LOG(AS_LOG_WARNING, "nSize = %d.", nSize);
        #endif
        processedSize += (uint32_t) nSize;
    }while (processedSize < totalSize);

    int32_t dataSize = m_ulRecvSize - processedSize;
    #ifdef _DEBUG_AVS
        MK_LOG(AS_LOG_WARNING, "m_ulRecvSize = %d, processedSize = %d , dataSize = %d.", m_ulRecvSize, processedSize, dataSize);
    #endif
    if(0 < dataSize) 
    {
        memmove(&m_RecvTcpBuf[0],&m_RecvTcpBuf[processedSize], dataSize);
    }
    m_ulRecvSize = dataSize;
    setHandleRecv(AS_TRUE);
    return;
}

void mk_rtsp_connection::handle_send(void)
{
    setHandleSend(AS_FALSE);
}

int32_t mk_rtsp_connection::handle_rtp_packet(MK_RTSP_HANDLE_TYPE type, as_msg_block* block)
{
    if (!m_pFormatCtx || !block)
        return AS_ERROR_CODE_FAIL;
    
    int32_t ret = 0;
    MK_Stream* stream1 = nullptr;
   
    for (int index = 0; index < m_pFormatCtx->streams.size(); ++index) 
    {
        auto stream = m_pFormatCtx->streams.at(index);
        if (!stream || !stream->track)
            continue;
            
        if (stream->interleaved_max >= type && stream->interleaved_min >= type)
        {
            stream1 = stream;
            break;
        }
    }
    
    if (!stream1)
    {
        AS_LOG(AS_LOG_WARNING, "rtsp connection could't found stream to support %d channel.", type);
        ret = AS_ERROR_CODE_INVALID;
    }
    else
    {
        ret = stream1->track->handleRtpPacket(block);
    }
    return ret;
}

int32_t mk_rtsp_connection::handle_rtcp_packet(MK_RTSP_HANDLE_TYPE type, as_msg_block* block)
{
    mk_rtsp_service::instance().free_rtp_recv_buf(block);
    return AS_ERROR_CODE_OK;
}

int32_t findInterleave(const char* pData, uint32_t unDataSize)
{
    uint32_t ulMsgLen = 0;
    uint32_t unMediaSize = 0;

    for (int i = 1; i < unDataSize - 1; ++i)
    {
        if (RTSP_INTERLEAVE_FLAG == pData[i])
        {
            uint8_t interleaveNum = pData[i + 1];
            if (interleaveNum < RTSP_INTERLEAVE_NUM_MAX)
            {
                if (i <= (unDataSize - RTSP_INTERLEAVE_HEADER_LEN))
                {
                    unMediaSize = (uint32_t)ntohs(*(uint16_t*)(void*)&pData[i + 2]);
                    if (unMediaSize > BIG_MSG_BLOCK_SIZE)
                    {
                        MK_LOG(AS_LOG_INFO, " rtp/rtcp payload size:[%d] is too large, continue.", unMediaSize);
                        continue;
                    }
                }

                ulMsgLen = i;
                break;
            }
        }
    }
    return ulMsgLen;
}

int32_t mk_rtsp_connection::processRecvedMessage(const char* pData, uint32_t unDataSize)
{
    if ((NULL == pData) || (0 == unDataSize))
        return AS_ERROR_CODE_FAIL;
    
    switch (m_Status)
    {
    case RTSP_SOCK5_PROXY_HANDSHAKE: 
    {
        if (unDataSize < 2) 
            return unDataSize;
       
        if (0x05 != pData[0])
        {
            AS_LOG(AS_LOG_ERROR, "socks5 version mismatch %d.", pData[0]);
            return AS_ERROR_CODE_INVALID;
        }
        if ((!m_bAuth && 0x00 == pData[1]) || m_bAuth) 
        {
            if (m_bAuth) 
                m_bAuth = static_cast<char>(SOCKS5_AUTH_TYPES::USERPASS) == pData[1] ? true : false;
            
            if (m_bAuth) 
            {
                m_Status = RTSP_SOCK5_PROXY_AUTH;
                if (AS_ERROR_CODE_OK != socks5Auth())
                    return AS_ERROR_CODE_FAIL;
            }
            else 
            {
                m_Status = RTSP_SOCK5_PROXY_REQUEST;
                if (AS_ERROR_CODE_OK != socks5ConnectionRequest())
                    return AS_ERROR_CODE_FAIL;
            }
            return unDataSize;
        }
        AS_LOG(AS_LOG_ERROR, "socks5 handshake failed.");
        return AS_ERROR_CODE_FAIL;
    }
    case RTSP_SOCK5_PROXY_AUTH:
    {
        if (unDataSize < 2) 
            return unDataSize;
        
        if (static_cast<char>(SOCKS5_DEFAULTS::VER_USERPASS) == pData[0] && 0x00 == pData[1]) 
        {
            m_Status = RTSP_SOCK5_PROXY_REQUEST;
            if (AS_ERROR_CODE_OK != socks5ConnectionRequest())
                return AS_ERROR_CODE_FAIL;
            return unDataSize;
        }
        AS_LOG(AS_LOG_ERROR, "socks5 auth failed.");
        return AS_ERROR_CODE_FAIL;
    }
    case RTSP_SOCK5_PROXY_REQUEST:
    {
        if (0x05 == pData[0] &&  0x00 == pData[1]) 
        {
            m_Status = RTSP_SESSION_STATUS_INIT;
            if (m_bVoiceTalk)
                sendRtspDescribeReq();
            else 
                sendRtspOptionsReq();
            return unDataSize;
        }
        return AS_ERROR_CODE_FAIL;
    }
    default:
        break;
    }

    if (kFoundInterleave == m_recvState)
    {
        uint32_t interleave = findInterleave(pData, unDataSize);
        if (interleave)
        {
            m_recvState = kNormal;
            return interleave;
        }
        else
        {
            AS_LOG(AS_LOG_INFO, "found interleave fail, continue.");
            return unDataSize;
        }
    }

    if (RTSP_INTERLEAVE_FLAG == pData[0]) 
    {
        if(m_bSendRtcp) 
        {
            time_t cur = time(NULL);
            if(MK_CLIENT_SEND_RTCP_INTERVAL <= (cur - m_ulLastRtcpSend)) 
            {
                sendRtcpMessage();
                m_ulLastRtcpSend = cur;
            }
        }
        return handleRtpRtcpData(pData, unDataSize);
    }
    /* rtsp message */
    mk_rtsp_packet rtspPacket;
    uint32_t ulMsgLen  = 0;
    m_ulStatusCode = 0;

    int32_t nRet = rtspPacket.checkRtsp(pData,unDataSize,ulMsgLen);
    if (AS_ERROR_CODE_INVALID == nRet)
    {
        ulMsgLen = unDataSize;
        uint32_t unMediaSize = 0;
        if (RTSP_SESSION_STATUS_PLAY == m_Status)
        {
            int32_t interleavePos = findInterleave(pData, unDataSize);
            if (0 == interleavePos)
            {
                m_recvState = kFoundInterleave;
                AS_LOG(AS_LOG_WARNING, "invalid method, not found INTERLEAVE_FLAG, drop all.");
            }
            else
            {
                ulMsgLen = interleavePos;
                m_recvState = kNormal;
                if (ulMsgLen <= (unDataSize - RTSP_INTERLEAVE_HEADER_LEN))
                {
                    AS_LOG(AS_LOG_INFO, "invalid method, found INTERLEAVE_FLAG at index %d, data [0x%02x], [0x%02x], datasize: %d.",
                        ulMsgLen, pData[ulMsgLen], pData[ulMsgLen + 1], unDataSize);
                }
            }
        }
        return ulMsgLen;
    }

    if (AS_ERROR_CODE_OK != nRet)
    {
        m_ulStatusCode = RTSP_STATUS_UNSUPPORTED_OPTION;
        MK_LOG(AS_LOG_WARNING,"rtsp connection check rtsp message fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if(0 == ulMsgLen)
    {
        MK_LOG(AS_LOG_DEBUG,"rtsp connection need recv more data.");
        return 0; /* need more data deal */
    }

    nRet = rtspPacket.parse(pData,unDataSize);
    m_ulStatusCode = rtspPacket.getRtspStatusCode();
    if (AS_ERROR_CODE_OK != nRet) 
    {
        MK_LOG(AS_LOG_WARNING,"rtsp connection parser rtsp message fail, rtsp status [%s].", rtspPacket.getRtspStatusString().c_str());
        return AS_ERROR_CODE_FAIL;
    }

    uint32_t method = rtspPacket.getMethodIndex();
    if (m_sessionId.empty() && !rtspPacket.getSessionID().empty())
        m_sessionId = rtspPacket.getSessionID();
	
    switch (method)
    {
        case RtspResponseMethod:
        {
            nRet = handleRtspResp(rtspPacket);
            break;
        }
        case RtspAnnounceMethod:
        {
            nRet = handleAnnounceReq(rtspPacket);
            if (AS_ERROR_CODE_OK != nRet) 
            {
                handle_connection_status(MR_CLIENT_STATUS_EOS);
                nRet = AS_ERROR_CODE_OK;
            }
            break;
        }
        case RtspPlayNotifyMethod:
        {
            handle_connection_status(MR_CLIENT_STATUS_EOS);
            nRet = AS_ERROR_CODE_OK;
            break;
        }
        case RtspSetParameterMethod:
        {
            handleSetParameterReq(rtspPacket);
            break;
        }
        default:
        {
            nRet = AS_ERROR_CODE_FAIL;
            break;
        }
    }

    if(AS_ERROR_CODE_OK != nRet) 
    {
        MK_LOG(AS_LOG_WARNING,"rtsp connection process method [%s] fail.", mk_rtsp_packet::getMethodString(method).c_str());
        return AS_ERROR_CODE_FAIL;
    }
    return ulMsgLen;
}

int32_t mk_rtsp_connection::handleRtpRtcpData(const char* pData, uint32_t unDataSize)
{
    if (unDataSize < RTSP_INTERLEAVE_HEADER_LEN) {
        return 0;
	}
    
    uint32_t unMediaSize = (uint32_t)ntohs(*(uint16_t*)(void*)&pData[2]);
    if (unDataSize - RTSP_INTERLEAVE_HEADER_LEN < unMediaSize) {
        return 0;
    }

    int32_t magicIndex = 0;
    uint8_t channel = (uint8_t)pData[magicIndex + 1];
    uint8_t version = (uint8_t)(pData[4]) >> 6;
    if( ((RTSP_INTERLEAVE_NUM_MAX <= channel) && (2 != version)) || (BIG_MSG_BLOCK_SIZE < unMediaSize))
    {
        MK_LOG(AS_LOG_INFO," rtp/rtcp channel or size:[%d] [%d] is invalid, try to find next interleave flag(0x24) in %ds data.", 
                channel, unMediaSize, unDataSize);

        uint32_t interleavePos = findInterleave(pData, unDataSize);
        if (0 == interleavePos)
        {
            m_recvState = kFoundInterleave;
            MK_LOG(AS_LOG_WARNING, "could't find interleave flag, drop all.");
            return unDataSize;
        }
        magicIndex = interleavePos;
        m_recvState = kNormal;

        if (magicIndex <= (unDataSize - RTSP_INTERLEAVE_HEADER_LEN))
        {
            MK_LOG(AS_LOG_INFO, "found interleave flag at index %d in data [0x%02x], [0x%02x], datasize: %d.",
                magicIndex, pData[magicIndex], pData[magicIndex + 1], unMediaSize);
        }

        if (unDataSize - magicIndex < RTSP_INTERLEAVE_HEADER_LEN)
            return magicIndex;
        
        channel = (uint8_t)pData[magicIndex + 1];
        if (RTSP_INTERLEAVE_NUM_MAX <= channel)
        {
            m_ulStatusCode = RTPRTCP_PROCESS_ERR;
            m_recvState = kFoundInterleave;
            MK_LOG(AS_LOG_WARNING, " rtp/rtcp channel:[%d] is invalid again, fail.", channel);
            return AS_ERROR_CODE_FAIL;
        }
            
        if (unDataSize - magicIndex - RTSP_INTERLEAVE_HEADER_LEN < unMediaSize)
            return magicIndex;
    }

    as_msg_block* block = mk_rtsp_service::instance().get_rtp_recv_buf(unMediaSize);
	if (NULL == block) {
		MK_LOG(AS_LOG_ERROR, "no available packet block found, maybe memory is not release.")
			return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN + magicIndex);
	}

    //MK_LOG(AS_LOG_ERROR,"rtsp connection process rtp or rtcp size:[%d].",unMediaSize);
    if (block->write(&pData[magicIndex + RTSP_INTERLEAVE_HEADER_LEN], unMediaSize))
    {
        mk_rtsp_service::instance().free_rtp_recv_buf(block);
        return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN + magicIndex);
    }

    int nRet = 0;
    RTSP_INTERLEAVE_NUM interleave = RTSP_INTERLEAVE_NUM(pData[magicIndex + 1]);
    switch (interleave)
    {
    case RTSP_INTERLEAVE_NUM_VIDEO_RTP:
    case RTSP_INTERLEAVE_NUM_AUDIO_RTP:
    {
        nRet = this->handle_rtp_packet((MK_RTSP_HANDLE_TYPE)interleave, block);
        if(AS_ERROR_CODE_INVALID != nRet)
            m_ulLastRecv = time(NULL);
        break;
    }
    case RTSP_INTERLEAVE_NUM_VIDEO_RTCP:
    case RTSP_INTERLEAVE_NUM_AUDIO_RTCP:
        nRet = this->handle_rtcp_packet((MK_RTSP_HANDLE_TYPE)interleave, block);
        break;
    default:
        mk_rtsp_service::instance().free_rtp_recv_buf(block);
        break;
    }
    
    if(0 != nRet) 
    { 
        mk_rtsp_service::instance().free_rtp_recv_buf(block);
    }

    return (int32_t)(unMediaSize + RTSP_INTERLEAVE_HEADER_LEN + magicIndex);
}

int32_t mk_rtsp_connection::socks5Greeting()
{
    std::vector<char> client_greeting_msg;
    if (m_bAuth)
    {
        client_greeting_msg  = {
            static_cast<char>(SOCKS5_CGREETING_AUTH::VERSION),
            static_cast<char>(SOCKS5_CGREETING_AUTH::NAUTH),
            static_cast<char>(SOCKS5_AUTH_TYPES::USERPASS)
        };
    }
    else 
    {
        client_greeting_msg = {
            static_cast<char>(SOCKS5_CGREETING_NOAUTH::VERSION),
            static_cast<char>(SOCKS5_CGREETING_NOAUTH::NAUTH),
            static_cast<char>(SOCKS5_CGREETING_NOAUTH::AUTH)
        };
    }
    return sendMsg(client_greeting_msg.data(), client_greeting_msg.size());
}

int32_t mk_rtsp_connection::socks5Auth()
{
    std::vector<char> client_auth_request = {
        static_cast<char>(SOCKS5_DEFAULTS::VER_USERPASS),
        static_cast<char>(strlen(m_sockUrl.username))
    };

    for (std::size_t i = 0; i < strlen(m_sockUrl.username); i++) 
        client_auth_request.push_back(m_sockUrl.username[i]);

    client_auth_request.push_back(static_cast<char>(strlen(m_sockUrl.password)));
    for (std::size_t i = 0; i < strlen(m_sockUrl.password); i++)
        client_auth_request.push_back(m_sockUrl.password[i]);
 
    return sendMsg(client_auth_request.data(), client_auth_request.size());
}

int32_t mk_rtsp_connection::socks5ConnectionRequest()
{
    int32_t ret = 0;
    std::vector<char> client_conn_request = {
                static_cast<char>(SOCKS5_CGREETING_NOAUTH::VERSION),
                static_cast<char>(SOCKS5_CCONNECTION_CMD::TCP_IP_STREAM),
                static_cast<char>(SOCKS5_DEFAULTS::RSV),
                static_cast<char>(SOCKS5_ADDR_TYPE::SOCKS_DOMAIN),
                static_cast<char>(strlen(m_url.host))
    };

    for (std::size_t i = 0; i < strlen(m_url.host); i++)
        client_conn_request.push_back(m_url.host[i]);
 
    client_conn_request.push_back(static_cast<char>(m_url.port >> 8));
    client_conn_request.push_back(static_cast<char>(m_url.port));
    ret = sendMsg(client_conn_request.data(), client_conn_request.size());
    return ret;
}

int32_t mk_rtsp_connection::getCurConnectPort(sockaddr* addr, socklen_t addrlen, int32_t family)
{
	u_short port = -1;
	if (getsockname(m_lSockFD, addr, &addrlen) >= 0) {
		if (family == AF_INET)
		{
			struct sockaddr_in* sin = (struct sockaddr_in*)addr;
			port = ntohs(sin->sin_port);
		} 
        else if (family == AF_INET6)
		{
			struct sockaddr_in6* sin = (struct sockaddr_in6*)addr;
			port = ntohs(sin->sin6_port);
		}
	}
    return port;
}

int32_t mk_rtsp_connection::sendRtcpMessage()
{
    char buf[KILO] = { 0 };
    char* pRtcpBuff = buf + RTP_INTERLEAVE_LENGTH;
    uint32_t unRtcpLen = 0;

    TRANS_DIRECTION emDirect = m_sdpInfo.getTransDirect();

    if (TRANS_DIRECTION_SENDONLY == emDirect)
    {
        (void)m_rtcpPacket.createSenderReport(pRtcpBuff,
            KILO - RTP_INTERLEAVE_LENGTH,
            unRtcpLen);
    }
    else {
        (void)m_rtcpPacket.createReceiverReport(pRtcpBuff,
            KILO - RTP_INTERLEAVE_LENGTH,
            unRtcpLen);
    }

    buf[0] = RTP_INTERLEAVE_FLAG;
    buf[1] = (char)RTSP_INTERLEAVE_NUM_VIDEO_RTCP;
    *(uint16_t*)&buf[2] = htons((uint16_t)unRtcpLen);

    return sendMsg(buf, unRtcpLen + RTP_INTERLEAVE_LENGTH);
}

int32_t mk_rtsp_connection::sendRtspReq(RstpRequestStruct& rtspReq)
{
    std::string strUrl = std::string("rtsp://") + (char*)&m_url.host[0];
    if (0 != m_url.port) 
    {
        std::stringstream strPort;
        strPort << m_url.port;
        strUrl += ":" + strPort.str();
    }

    if ("" == rtspReq.m_url)
        strUrl += (char*)&m_url.uri[0];
    else 
        strUrl = rtspReq.m_url;

    mk_rtsp_packet rtspPacket;
    rtspPacket.setMethodIndex(rtspReq.m_method);
    rtspPacket.setCseq(m_ulSeq);
    rtspPacket.setRtspUrl(strUrl);
    if ("" != rtspReq.m_transport) 
        rtspPacket.setTransPort(rtspReq.m_transport);
  
    /* Authorization */
    if (('\0' != m_url.username[0]) && ('\0' != m_url.password[0]) && ("" != m_strAuthenticate)) 
    {
        char   Digest[RTSP_DIGEST_LENS_MAX] = { 0 };
        as_digest_attr_value_t value;
        value.string = (char*)&m_url.username[0];
        as_digest_set_attr(&m_Authen, D_ATTR_USERNAME, value);
        value.string = (char*)&m_url.password[0];
        as_digest_set_attr(&m_Authen, D_ATTR_PASSWORD, value);
        value.number = DIGEST_ALGORITHM_NOT_SET;
        as_digest_set_attr(&m_Authen, D_ATTR_ALGORITHM, value);

        value.string = (char*)strUrl.c_str();
        as_digest_set_attr(&m_Authen, D_ATTR_URI, value);
        //value.number = DIGEST_QOP_AUTH;
        //as_digest_set_attr(&m_Authen, D_ATTR_QOP, value);
        std::string strMethod = mk_rtsp_packet::getMethodString(rtspReq.m_method);
        value.string = (char*)strMethod.c_str();
        as_digest_set_attr(&m_Authen, D_ATTR_METHOD, value);

       /* MK_LOG(AS_LOG_INFO, "rtsp connection auth username:[%s] passwrod:[%s]\n"
            "url:[%s] method:[%s]",
            (char*)&m_url.username[0], (char*)&m_url.password[0],
            (char*)rtspReq.m_url.c_str(), strMethod.c_str());*/

        if (-1 == as_digest_client_generate_header(&m_Authen, Digest, sizeof(Digest))) 
        {
            MK_LOG(AS_LOG_INFO, "generate digest fail.");
            return AS_ERROR_CODE_FAIL;
        }
        MK_LOG(AS_LOG_INFO, "generate digest :[%s].", Digest);
        std::string strAuthorization = (char*)&Digest[0];
        rtspPacket.setAuthorization(strAuthorization);
    }

    if (!rtspReq.m_ulSession.empty()) 
        rtspPacket.setSessionID(rtspReq.m_ulSession);
    
    if (0 != rtspReq.m_scale)
        rtspPacket.setScale(rtspReq.m_scale);

    if (0 != rtspReq.m_speed) 
        rtspPacket.setSpeed(rtspReq.m_speed);

    if (rtspReq.m_start >= 0) 
        rtspPacket.setRangeTime(RELATIVE_TIME,rtspReq.m_start, rtspReq.m_end);

    std::string m_strMsg;
    if (AS_ERROR_CODE_OK != rtspPacket.generateRtspReq(rtspReq.m_method, m_strMsg)) 
    {
        MK_LOG(AS_LOG_INFO, "rtsp generate Rtsp Request fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_SeqMethodMap.insert(SEQ_METHOD_MAP::value_type(m_ulSeq, rtspReq.m_method));
    m_ulSeq++;
    MK_LOG(AS_LOG_INFO, "rtsp connection send request:\n%s", m_strMsg.c_str());
    return sendMsg(m_strMsg.c_str(), m_strMsg.length());
}

int32_t mk_rtsp_connection::sendRtspReq(enRtspMethods enMethod, std::string& strUri, std::string& strTransport, std::string ullSessionId, double scale)
{
    RstpRequestStruct rtspRequest(enMethod,strUri,strTransport,ullSessionId,scale);
    return sendRtspReq(rtspRequest);
}

int32_t mk_rtsp_connection::sendRtspOptionsReq()
{
    return sendRtspReq(RtspOptionsMethod,STR_NULL,STR_NULL);
}

int32_t mk_rtsp_connection::sendRtspDescribeReq()
{
    return sendRtspReq(RtspDescribeMethod,STR_NULL,STR_NULL);
}

int32_t mk_rtsp_connection::sendRtspSetupReq(SDP_MEDIA_INFO* info)
{
    std::string strTransport = "";

    // (RTP)
    strTransport += RTSP_TRANSPORT_RTP;
    strTransport += RTSP_TRANSPORT_SPEC_SPLITER;
    strTransport += RTSP_TRANSPORT_PROFILE_AVP;

    //(TCP/UDP)
    if (m_bSetupTcp) {
        strTransport += RTSP_TRANSPORT_SPEC_SPLITER;
        strTransport += RTSP_TRANSPORT_TCP;
    }
    strTransport += SIGN_SEMICOLON;

    if (m_bSetupTcp) {
        strTransport += RTSP_TRANSPORT_UNICAST;
        strTransport += SIGN_SEMICOLON;
        strTransport += RTSP_TRANSPORT_INTERLEAVED;
        std::stringstream strChannelNo;
        if(MEDIA_TYPE_VALUE_VIDEO == info->ucMediaType) {
            strChannelNo << RTSP_INTERLEAVE_NUM_VIDEO_RTP;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info->ucMediaType){
            strChannelNo << RTSP_INTERLEAVE_NUM_AUDIO_RTP;
        }

        strTransport += strChannelNo.str() + SIGN_H_LINE;

        strChannelNo.str("");
        if(MEDIA_TYPE_VALUE_VIDEO == info->ucMediaType) {
            strChannelNo << RTSP_INTERLEAVE_NUM_VIDEO_RTCP;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info->ucMediaType){
            strChannelNo << RTSP_INTERLEAVE_NUM_AUDIO_RTCP;
        }
        strTransport += strChannelNo.str();
    }
    else {
        strTransport += RTSP_TRANSPORT_UNICAST;
        strTransport += SIGN_SEMICOLON;

        mk_rtsp_udp_handle*  pRtpHandle  = NULL;
        mk_rtsp_udp_handle* pRtcpHandle = NULL;
        MK_RTSP_HANDLE_TYPE rtpType = MK_RTSP_UDP_TYPE_MAX;
        MK_RTSP_HANDLE_TYPE rtcpType = MK_RTSP_UDP_TYPE_MAX;

        if(AS_ERROR_CODE_OK != mk_rtsp_service::instance().get_rtp_rtcp_pair(pRtpHandle,pRtcpHandle))
        {
            MK_LOG(AS_LOG_ERROR,"rtsp connection client get rtp and rtcp handle for udp fail.");
            return AS_ERROR_CODE_FAIL;
        }

        if(MEDIA_TYPE_VALUE_VIDEO == info->ucMediaType) {
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_VIDEO_RTCP_HANDLE]  = pRtcpHandle;
            rtpType = MK_RTSP_UDP_VIDEO_RTP_HANDLE;
            rtcpType = MK_RTSP_UDP_VIDEO_RTCP_HANDLE;
        }
        else if(MEDIA_TYPE_VALUE_AUDIO == info->ucMediaType) {
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTP_HANDLE]   = pRtpHandle;
            m_udpHandles[MK_RTSP_UDP_AUDIO_RTCP_HANDLE]  = pRtcpHandle;
            rtpType = MK_RTSP_UDP_AUDIO_RTP_HANDLE;
            rtcpType = MK_RTSP_UDP_AUDIO_RTCP_HANDLE;
        }
        else  {
            MK_LOG(AS_LOG_ERROR,"rtsp connection setup error,the media type is not rtp and rtcp.");
            return AS_ERROR_CODE_FAIL;
        }

		if (AS_ERROR_CODE_OK != pRtpHandle->start_handle(rtpType, this)) {
            return AS_ERROR_CODE_FAIL;
        }
		if (AS_ERROR_CODE_OK != pRtcpHandle->start_handle(rtcpType, this)) {
            return AS_ERROR_CODE_FAIL;
        }

        strTransport += RTSP_TRANSPORT_CLIENT_PORT;
        std::stringstream strPort;
        strPort << pRtpHandle->get_port();
        strTransport += strPort.str() + SIGN_H_LINE;
        strPort.str("");
        strPort << pRtcpHandle->get_port();
        strTransport += strPort.str();
    }

    MK_LOG(AS_LOG_INFO,"rtsp connection client ,setup control:[%s].",info->strControl.c_str());

    std::string strUri = STR_NULL;
    if("" != info->strControl && !strncmp(info->strControl.c_str(), RTSP_URL_PREFIX, strlen(RTSP_URL_PREFIX)) ) {
        strUri = info->strControl;
    }
    else if ("" != m_strConBase && !strncmp(m_strConBase.c_str(), RTSP_URL_PREFIX, strlen(RTSP_URL_PREFIX))) {
        strUri = m_strConBase;
        if ('/' != *(--m_strConBase.end())) {
            strUri += "/";
        }
        if ("" != info->strControl && !strncmp(info->strControl.c_str(), RTSP_TRCK_PREFIX, strlen(RTSP_TRCK_PREFIX))) {
            strUri += info->strControl;
        }
	} else if ("" != info->strControl && !strncmp(info->strControl.c_str(), RTSP_TRCK_PREFIX, strlen(RTSP_TRCK_PREFIX))) {
		std::string Url = std::string("rtsp://") + (char*)&m_url.host[0];
		if (0 != m_url.port) {
			std::stringstream strPort;
			strPort << m_url.port;
			Url += ":" + strPort.str();
		}
		Url += (char*)&m_url.uri[0];
		Url += "/";
		Url += info->strControl;
		strUri = Url;
	}
    
    return sendRtspReq(RtspSetupMethod,strUri,strTransport, m_sessionId);
}

int32_t mk_rtsp_connection::sendRtspPlayReq(double start,double scale, double speed)
{
    RstpRequestStruct playControl(RtspPlayMethod, STR_NULL, STR_NULL, m_sessionId ,scale, speed, start);
    return sendRtspReq(playControl);
}

int32_t mk_rtsp_connection::sendRtspRecordReq()
{
    return sendRtspReq(RtspRecordMethod,STR_NULL,STR_NULL, m_sessionId);
}

int32_t mk_rtsp_connection::sendRtspGetParameterReq()
{
    return sendRtspReq(RtspGetParameterMethod,STR_NULL,STR_NULL);
}

int32_t mk_rtsp_connection::sendRtspAnnounceReq()
{
    return sendRtspReq(RtspAnnounceMethod,STR_NULL,STR_NULL);
}

int32_t mk_rtsp_connection::sendRtspPauseReq()
{
    return sendRtspReq(RtspPauseMethod,STR_NULL,STR_NULL, m_sessionId);
}

int32_t mk_rtsp_connection::sendRtspTeardownReq()
{
    return sendRtspReq(RtspTeardownMethod,STR_NULL,STR_NULL, m_sessionId);
}

int32_t mk_rtsp_connection::initTracks()
{
    int32_t ret = 0;
    MK_Stream* stream;
    for (int index = 0; index < m_pFormatCtx->streams.size(); ++index) {
        if (stream = m_pFormatCtx->streams.at(index)) {
            AS_DELETE(stream->track);
            switch (stream->codecpar->codec_id){
            case MK_CODEC_ID_H264:
                if(!(stream->track = new H264Track(this, index)))
                    ret = AS_ERROR_CODE_MEM;
                break;
            case MK_CODEC_ID_HEVC:
                if (!(stream->track = new HEVCTrack(this, index)))
                    ret = AS_ERROR_CODE_MEM;
                break;
            case MK_CODEC_ID_MJPEG:
                if(!(stream->track = new MJPEGTrack(this, index)))
                    ret = AS_ERROR_CODE_MEM;
                break;
            case MK_CODEC_ID_PCM_MULAW:
            case MK_CODEC_ID_PCM_ALAW:
                if (!(stream->track = new G711Track(this, index)))
                    ret = AS_ERROR_CODE_MEM;
                break;
            case MK_CODEC_ID_AAC:
                if (!(stream->track = new AACTrack(this, index)))
                    ret = AS_ERROR_CODE_MEM;
                break;
            default:
                ret = AS_ERROR_CODE_INVALID;
                break;
            }
            
            if (!ret && stream && stream->track) {
                ret = stream->track->init();
            }
            if (ret && AS_ERROR_CODE_INVALID != ret) {
                break;
            }
        }
    }

    if (ret && AS_ERROR_CODE_INVALID != ret) {
        m_pFormatCtx->release();
        AS_DELETE(m_pFormatCtx);
    }
    return ret;
}

int32_t mk_rtsp_connection::getAddrInfo(struct addrinfo** ailist)
{
    struct            addrinfo hint;
    char*             pNodeName;
    hint.ai_family    = AF_UNSPEC;
    hint.ai_socktype  = SOCK_STREAM;
    hint.ai_flags     = AI_PASSIVE;  
    hint.ai_protocol  = IPPROTO_IP;         
    hint.ai_addrlen   = 0;          
    hint.ai_canonname = NULL;
    hint.ai_addr      = NULL;
    hint.ai_next      = NULL;

    char port[sizeof(m_url.port) + 10];

    if (m_bSocks)
    {
        pNodeName = (char*)&m_sockUrl.host[0];
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
		sprintf_s(port, "%d", m_sockUrl.port);
#else
		sprintf(port, "%d", m_sockUrl.port);
#endif
    }
    else 
    {
        pNodeName = (char*)&m_url.host[0];
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
		sprintf_s(port, "%d", m_url.port);
#else
		sprintf(port, "%d", m_url.port);
#endif
    }

    int ilRc = getaddrinfo(pNodeName, port, &hint, ailist);

    if ((ilRc == 0) && (ailist != NULL))
    {
        return AS_ERROR_CODE_OK;
    }

    return AS_ERROR_CODE_FAIL;
}

int32_t mk_rtsp_connection::handleAnnounceReq(mk_rtsp_packet& rtspMessage)
{
    std::string strSdp = "";
    std::string strContBase = "";
    rtspMessage.getContent(strSdp);
    m_strSdpInfo = strSdp;

    rtspMessage.getContentBase(strContBase);
    m_strConBase = strContBase;

    MK_LOG(AS_LOG_INFO, "rtsp client connection handle announce request sdp:[%s].", strSdp.c_str());

    int32_t ret = m_sdpInfo.decodeSdp(strSdp, m_strConBase);
    if (AS_ERROR_CODE_OK != ret) 
    {
        MK_LOG(AS_LOG_WARNING, "parse announce sdp:[%s] to media attribute fail.", strSdp.c_str());
        return AS_ERROR_CODE_FAIL;
    }
    
    m_mediaInfoList.clear();
    m_sdpInfo.getVideoInfo(m_mediaInfoList);
    m_sdpInfo.getAudioInfo(m_mediaInfoList);

    if (0 == m_mediaInfoList.size())
    {
        MK_LOG(AS_LOG_WARNING, "rtsp connection handle announce request fail,there is no media info.");
        return AS_ERROR_CODE_FAIL;
    }

    if (ret = initTracks())
        return ret;

    SDP_MEDIA_INFO* info = m_mediaInfoList.front();
    m_mediaInfoList.pop_front();
    MK_LOG(AS_LOG_INFO, "rtsp client connection handle announce request end.");
    return ret;
}

int32_t mk_rtsp_connection::handleSetParameterReq(mk_rtsp_packet& rtspMessage)
{
    std::string info = rtspMessage.getSetParamXInfo();
    if ("EOS" == info) {
        handle_connection_status(MR_CLIENT_STATUS_EOS);
    }
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_connection::handleRtspResp(mk_rtsp_packet &rtspMessage)
{
    uint32_t nCseq     = rtspMessage.getCseq();
    uint32_t nRespCode = rtspMessage.getRtspStatusCode();
    uint32_t enMethod   = RtspIllegalMethod;

    MK_LOG(AS_LOG_INFO,"rtsp client handle server reponse seq:[%d] ,Response Code:[%d].",nCseq,nRespCode);

    SEQ_METHOD_ITER iter = m_SeqMethodMap.find(nCseq);
    if(iter == m_SeqMethodMap.end()) {
        MK_LOG(AS_LOG_WARNING,"rtsp client there server reponse, not find the request Cseq:[%d].",nCseq);
        return AS_ERROR_CODE_FAIL;
    }

    enMethod = iter->second;
    MK_LOG(AS_LOG_INFO,"rtsp client handle server reponse seq:[%d] ,mothod:[%s].",nCseq, mk_rtsp_packet::getMethodString(enMethod).c_str());

    if(RTSP_STATUS_UNAUTHORIZED == nRespCode) 
    {
        if(RTSP_AUTH_TRY_MAX < m_ulAuthenTime) 
        {
            MK_LOG(AS_LOG_WARNING,"rtsp server need Authen, but try:[%d] time,so error auth info.",m_ulAuthenTime);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != rtspMessage.getAuthenticate(m_strAuthenticate))
        {
            MK_LOG(AS_LOG_WARNING,"rtsp server need Authen, get Authenticate header fail.");
            return AS_ERROR_CODE_FAIL;
        }
        MK_LOG(AS_LOG_INFO,"rtsp client handle authInfo:[%s].",m_strAuthenticate.c_str());
        if (AS_ERROR_CODE_OK != as_digest_is_digest(m_strAuthenticate.c_str()))
        {
            MK_LOG(AS_LOG_WARNING,"the WWW-Authenticate is not digest.");
            return AS_ERROR_CODE_FAIL;
        }

        if (AS_ERROR_CODE_OK == as_digest_client_parse(&m_Authen, m_strAuthenticate.c_str())) 
        {
            MK_LOG(AS_LOG_WARNING,"parser WWW-Authenticate info fail.");
            return AS_ERROR_CODE_FAIL;
        }
        m_ulAuthenTime++;
        MK_LOG(AS_LOG_INFO,"rtsp server need Authen, send request with Authorization info,time:[%d].",m_ulAuthenTime);
        return sendRtspReq((enRtspMethods)enMethod,STR_NULL,STR_NULL);
    }
    else if(RTSP_STATUS_OK != nRespCode) 
    {
        /*close socket */
        MK_LOG(AS_LOG_WARNING,"rtsp client there server reponse code:[%d].",nRespCode);
        return AS_ERROR_CODE_FAIL;
    }

    int nRet = AS_ERROR_CODE_OK;
    switch (enMethod)
    {
        case RtspDescribeMethod:
        {
            nRet = handleRtspDescribeResp(rtspMessage);
            break;
        }
        case RtspSetupMethod:
        {
            nRet = handleRtspSetUpResp(rtspMessage);
            break;
        }        
        case RtspTeardownMethod:
        {
            //nothing to do close the socket
            m_Status = RTSP_SESSION_STATUS_TEARDOWN;
            handle_connection_status(MR_CLIENT_STATUS_TEARDOWN);
            break;
        }
        case RtspPlayMethod:
        {
            //start rtcp and media check timer
            m_Status = RTSP_SESSION_STATUS_PLAY;
            handle_connection_status(MR_CLIENT_STATUS_RUNNING);
            break;
        }
        case RtspPauseMethod:
        {
            //nothing to do
            m_Status = RTSP_SESSION_STATUS_PAUSE;
            handle_connection_status(MR_CLIENT_STATUS_PAUSE);
            break;
        }
        case RtspOptionsMethod:
        {
            if(RTSP_SESSION_STATUS_INIT == m_Status) {
                nRet = sendRtspDescribeReq();
            }
            break;
        }
        case RtspAnnounceMethod:
        {
            //TODO: 
            break;
        }
        case RtspGetParameterMethod:
        {
            //nothing to do
            break;
        }
        case RtspSetParameterMethod:
        {
            //nothing to do
            break;
        }
        case RtspRedirectMethod:
        {
            //nothing to do
            break;
        }
        case RtspRecordMethod:
        {
            //rtsp client push media to server
            m_Status = RTSP_SESSION_STATUS_PLAY;
            break;
        }
        case RtspPlayNotifyMethod:
        {
            break;
        }
        case RtspResponseMethod:
        {
            //nothing to do
            break;
        }
        default:
        {
            nRet = AS_ERROR_CODE_FAIL;
            break;
        }
    }
    MK_LOG(AS_LOG_INFO,"rtsp client handle server reponse end.");
    return nRet;
}

int32_t mk_rtsp_connection::handleRtspDescribeResp(mk_rtsp_packet &rtspMessage)
{
    std::string strSdp = "";
	std::string strContBase = "";
    rtspMessage.getContent(strSdp);
    m_strSdpInfo = strSdp;

	rtspMessage.getContentBase(strContBase);
	m_strConBase = strContBase;

    MK_LOG(AS_LOG_INFO,"rtsp client connection handle describe response sdp:[%s].",strSdp.c_str());

	int32_t ret = m_sdpInfo.decodeSdp(strSdp, m_strConBase);
    if(AS_ERROR_CODE_OK != ret) {
        MK_LOG(AS_LOG_WARNING,"rtsp connection handle describe sdp:[%s] fail.",strSdp.c_str());
        return AS_ERROR_CODE_FAIL;
    }
   
    m_mediaInfoList.clear();
    m_sdpInfo.getVideoInfo(m_mediaInfoList);
    m_sdpInfo.getAudioInfo(m_mediaInfoList);

    if(0 == m_mediaInfoList.size()) {
        MK_LOG(AS_LOG_WARNING,"rtsp connection handle describe response fail,there is no media info.");
        return AS_ERROR_CODE_FAIL;
    }

    if (AS_ERROR_CODE_OK != (ret = initTracks())) {
        return ret;
    }

    SDP_MEDIA_INFO* info = m_mediaInfoList.front();
    ret = sendRtspSetupReq(info);
    if(AS_ERROR_CODE_OK != ret) 
    {
        MK_LOG(AS_LOG_WARNING,"rtsp connection handle describe response,send setup fail.");
        return AS_ERROR_CODE_FAIL;
    }
    m_mediaInfoList.pop_front();
    MK_LOG(AS_LOG_INFO,"rtsp client connection handle describe response end.");
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtsp_connection::handleRtspSetUpResp(mk_rtsp_packet &rtspMessage)
{
    MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response begin.");
    int32_t nRet = AS_ERROR_CODE_OK;
    if(0 < m_mediaInfoList.size())
    {
        MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response,send next setup messgae.");
        SDP_MEDIA_INFO* info = m_mediaInfoList.front();
        nRet = sendRtspSetupReq(info);
        if(AS_ERROR_CODE_OK != nRet) {
            MK_LOG(AS_LOG_WARNING,"rtsp connection handle setup response,send next setup fail.");
            return AS_ERROR_CODE_FAIL;
        }
        m_mediaInfoList.pop_front();
    }
    else 
    {
        m_Status = RTSP_SESSION_STATUS_SETUP;
        handle_connection_status(MR_CLIENT_STATUS_HANDSHAKE);
        /* send play */
//        MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response,sessionid:[%lld],send play messgae.",rtspMessage.getSessionID());
		if (kScale == (SpeedEnum)m_vcrSt.scaleOrSpeed) {
			nRet = sendRtspPlayReq(m_vcrSt.start, m_vcrSt.scale);
		} else if (kSpeed == (SpeedEnum)m_vcrSt.scaleOrSpeed) {
			nRet = sendRtspPlayReq(m_vcrSt.start, 0.0, m_vcrSt.scale);
		}
        m_bSetupSuccess = true;
    }
    MK_LOG(AS_LOG_INFO,"rtsp client connection handle setup response end.");
    return nRet;
}

int32_t mk_rtsp_connection::sendMsg(const char* pszData,uint32_t len)
{
    int32_t lSendSize = as_tcp_conn_handle::send(pszData,len,enSyncOp);
    if(lSendSize != len) 
    {
        MK_LOG(AS_LOG_WARNING,"rtsp connection send msg len:[%d] return:[%d] fail.",len,lSendSize);
        return AS_ERROR_CODE_FAIL;
    }
    return AS_ERROR_CODE_OK;
}

void mk_rtsp_connection::resetRtspConnect()
{
    //m_rtpFrameOrganizer.release();
}

void mk_rtsp_connection::trimString(std::string& srcString) const
{
    string::size_type pos = srcString.find_last_not_of(' ');
    if (pos != string::npos)
    {
        (void) srcString.erase(pos + 1);
        pos = srcString.find_first_not_of(' ');
        if (pos != string::npos)
            (void) srcString.erase(0, pos);
    }
    else {
        (void)srcString.erase(srcString.begin(), srcString.end());
    }
    return;
}

/*******************************************************************************************************
 * 
 * 
 *
 * ****************************************************************************************************/

mk_rtsp_server::mk_rtsp_server()
{
    m_cb  = NULL;
    m_ctx = NULL;
}

mk_rtsp_server::~mk_rtsp_server()
{
}

void mk_rtsp_server::set_callback(rtsp_server_request cb,void* ctx)
{
    m_cb = cb;
    m_ctx = ctx;
}

int32_t mk_rtsp_server::handle_accept(const as_network_addr *pRemoteAddr, as_tcp_conn_handle *&pTcpConnHandle)
{
    return AS_ERROR_CODE_OK;
}

mk_rtsp_connection::RstpRequestStruct::RstpRequestStruct(enRtspMethods enMethod, std::string strUri, std::string strTransport,
    std::string ullSessionId, float scale, float speed, double start, double end)
    :m_method(enMethod),m_url(strUri),m_transport(strTransport),
     m_ulSession(ullSessionId), m_scale(scale),m_speed(speed),m_start(start),m_end(end)
{
}
