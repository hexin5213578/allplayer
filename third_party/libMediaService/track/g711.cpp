#include "g711.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_media_sdp.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_IOS)
#include "as_frame.h"
#endif

int G711Track::handleRtpPacket(as_msg_block* block) {
    if (abs(m_conn->get_client_frags()) > 1) {
        AS_LOG(AS_LOG_WARNING, "couldn't parse g711 audio rtp packet in multi_streams.");
        return AS_ERROR_CODE_INVALID;
    }

    char* buff = block->base();
    auto len = block->length();

    int ret = 0;
    mk_rtp_packet rtp;
    if (ret = rtp.ParsePacket(buff, len)) {
        return ret;
    }

    if (ret = checkValid(rtp))  {           // a/u »¥×ª
        MK_Stream* stream = nullptr;
        int adapt = 0;

        if (!(stream = getTrackStream()))
            return AS_ERROR_CODE_FAIL;

        if (PT_TYPE_PCMU == rtp.GetPayloadType()) {
            stream->payload = PT_TYPE_PCMU;
            stream->codecpar->codec_id = MK_CODEC_ID_PCM_MULAW;
            adapt = 1;
        }
        else if (PT_TYPE_PCMA == rtp.GetPayloadType()) {
            stream->payload = PT_TYPE_PCMA;
            stream->codecpar->codec_id = MK_CODEC_ID_PCM_ALAW;
            adapt = 1;
        }
        
        if (adapt) {
            m_conn->handle_connection_status(MK_CLENT_PT_ADAPT);
        }
        else {
            AS_LOG(AS_LOG_WARNING, "adapt g711 format fail.");
            return  AS_ERROR_CODE_FAIL;
        }
    }

    if (rtp.GetMarker()) {
        ret = handleRtp((uint8_t*)buff, len);
        mk_rtsp_service::instance().free_rtp_recv_buf(block);
        return 0;
    }
    return m_organizer.insertRtpPacket(block);
}

int32_t G711Track::handleRtp(uint8_t* rtpData, uint32_t rtpLen)
{
    mk_rtp_packet rtpPacket;
    if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket((char*)rtpData, rtpLen)) 
    {
        AS_LOG(AS_LOG_ERROR, "fail to send g711 rtp packet, parse rtp packet fail.");
        return AS_ERROR_CODE_FAIL;
    }

    auto rtpHeadLen = rtpPacket.GetHeadLen();
    auto timeStam = rtpPacket.GetTimeStamp();
    auto rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();
    uint32_t bufflen;

    auto buf = m_conn->handle_connection_databuf(rtpPayloadLen, bufflen);
    if (!buf) 
    {
        AS_LOG(AS_LOG_ERROR, "fail handle g711 rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_FAIL;
    }

    memcpy(buf, &rtpData[rtpHeadLen], rtpPayloadLen);
    MediaDataInfo dataInfo;
    memset(&dataInfo, 0, sizeof(dataInfo));
    packMediaInfo(dataInfo, 0, timeStam, 0);
    m_conn->handle_connection_media(&dataInfo, rtpPayloadLen);
    return 0;
}

//counld't be called normally
int32_t G711Track::handleRtpList(RTP_PACK_QUEUE& rtpFrameList)
{
    if (0 == rtpFrameList.size()) {
        return AS_ERROR_CODE_FAIL;
    }

    mk_rtp_packet rtpPacket;
    uint32_t      rtpLen, rtpHeadLen, rtpPayloadLen, timeStam;
    char* rtpData;

    uint32_t ulTotaldatalen = checkFrameTotalDataLen(rtpFrameList), bufflen;
    char* recBuf = m_conn->handle_connection_databuf(ulTotaldatalen, bufflen);

    if (!recBuf) {
        AS_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_MEM;
    }

    uint32_t m_ulRecvLen = 0;
    for (uint32_t i = 0; i < rtpFrameList.size(); i++) 
    {
        rtpData = rtpFrameList[i].pRtpMsgBlock->base();
        rtpLen = rtpFrameList[i].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(rtpData, rtpLen)) 
        {
            AS_LOG(AS_LOG_ERROR, "fail to send auido rtp packet, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }
        rtpHeadLen = rtpPacket.GetHeadLen();
        rtpData = &rtpData[rtpHeadLen];
        rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();
        timeStam = rtpPacket.GetTimeStamp();
        memcpy(&recBuf[m_ulRecvLen], rtpData, rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;
    }
    MediaDataInfo dataInfo;
    memset(&dataInfo, 0x0, sizeof(dataInfo));
    packMediaInfo(dataInfo, 0, timeStam, 0);
    m_conn->handle_connection_media(&dataInfo, m_ulRecvLen);
    return AS_ERROR_CODE_OK;
}
