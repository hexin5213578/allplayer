#include "aac.h"
#include "mk_rtsp_rtp_packet.h"
#include "adts.h"

int AACTrack::handleRtpPacket(as_msg_block* block)
{
    if (abs(m_conn->get_client_frags()) > 1)
    {
        AS_LOG(AS_LOG_WARNING, "couldn't parse aac audio rtp packet in multi_streams.");
        return AS_ERROR_CODE_INVALID;
    }

    char* buff = block->base();
    auto len = block->length();

    int ret = 0;
    mk_rtp_packet rtp;
    if ((ret = rtp.ParsePacket(buff, len)) || (ret = checkValid(rtp)))
        return ret;

    if (rtp.GetMarker()) {
        ret = handleRtp((uint8_t*)buff, len);
        mk_rtsp_service::instance().free_rtp_recv_buf(block);
        return 0;
    }
    return m_organizer.insertRtpPacket(block);
}

int32_t AACTrack::handleRtp(uint8_t* rtpData, uint32_t rtpLen)
{
    mk_rtp_packet rtpPacket;
    if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket((char*)rtpData, rtpLen)) 
    {
        AS_LOG(AS_LOG_ERROR, "fail to send aac rtp packet, parse rtp packet fail.");
        return AS_ERROR_CODE_FAIL;
    }

    uint32_t ulRtpHeadLen = rtpPacket.GetHeadLen(); 
    int32_t nLenOut = 0, nAuHeaderOffset = ulRtpHeadLen;
    vector<uint32_t> vecAacFrameLen;
    
    uint16_t AU_HEADER_LENGTH = (((rtpData[nAuHeaderOffset] << 8) | rtpData[nAuHeaderOffset + 1]) >> 4);//首2字节表示Au-Header的长度，单位bit，所以除以16得到Au-Header字节数
    nAuHeaderOffset += 2;
    
    for (int i = 0; i < AU_HEADER_LENGTH; ++i) 
    {
        uint16_t AU_HEADER = ((rtpData[nAuHeaderOffset] << 8) | rtpData[nAuHeaderOffset + 1]);//之后的2字节是AU_HEADER
        uint32_t nAac = (AU_HEADER >> 3);//其中高13位表示一帧AAC负载的字节长度，低3位无用
        vecAacFrameLen.push_back(nAac);
        nAuHeaderOffset += 2;
    }
    uint32_t ulAudioLen = rtpLen - nAuHeaderOffset - rtpPacket.GetTailLen();

    uint8_t* pAacPayload = rtpData + nAuHeaderOffset;//真正AAC负载开始处
    uint32_t nAacPayloadOffset = 0;
    auto formatContex = m_conn->get_client_av_format();

    for (int j = 0; j < AU_HEADER_LENGTH; ++j) 
    {
        uint32_t nAac = vecAacFrameLen.at(j);
        //generate ADTS header
        SAacParam param(nAac, formatContex->streams[m_streamId]->codecpar->sample_rate, formatContex->streams[m_streamId]->codecpar->channels);
        CADTS adts;
        adts.init(param);

        /* send direct */
        uint32_t aacLen = adts.getBufSize() + nAac;
        uint32_t bufflen;
        char* recBuf = m_conn->handle_connection_databuf(aacLen, bufflen);
        if (!recBuf)
        {
            AS_LOG(AS_LOG_ERROR, "fail to handle rtp packet, alloc buffer short.");
            return AS_ERROR_CODE_FAIL;
        }

        //write ADTS header
        memcpy(recBuf + nLenOut, adts.getBuf(), adts.getBufSize());
        nLenOut += adts.getBufSize();

        //write AAC payload
        memcpy(recBuf + nLenOut, pAacPayload + nAacPayloadOffset, nAac);
        nLenOut += nAac;
        nAacPayloadOffset += nAac;
        if (nAacPayloadOffset > ulAudioLen)
        {
            AS_LOG(AS_LOG_WARNING, "fail to handle aac data, invalid len has dectected!");
            break;
        }

        MediaDataInfo dataInfo;
        memset(&dataInfo, 0, sizeof(dataInfo));
        packMediaInfo(dataInfo, 0, rtpPacket.GetTimeStamp());
        m_conn->handle_connection_media(&dataInfo, aacLen);
    }
    return AS_ERROR_CODE_OK;
}

int32_t AACTrack::handleRtpList(RTP_PACK_QUEUE& rtpFrameList)
{
    if (0 == rtpFrameList.size()) 
        return AS_ERROR_CODE_FAIL;
    
    memset(recvbuf_, 0, 4096);
    mk_rtp_packet rtpPacket;
    uint32_t      rtpLen, rtpHeadLen, rtpPayloadLen, timeStam;
    char* rtpData;

    uint32_t ulTotaldatalen = checkFrameTotalDataLen(rtpFrameList);
    uint32_t  bufflen;
    if (ulTotaldatalen > 4096)
    {
        AS_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_MEM;
    }

    uint32_t ulRecvLen = 0;
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
        memcpy(&recvbuf_[ulRecvLen], rtpData, rtpPayloadLen);
        ulRecvLen += rtpPayloadLen;
    }
    int32_t nLenOut = 0, nAuHeaderOffset = 0;
    vector<uint32_t> vecAacFrameLen;

    //首2字节表示Au-Header的长度，单位bit，所以除以16得到Au-Header字节数
    uint16_t AU_HEADER_LENGTH = (((recvbuf_[nAuHeaderOffset] << 8) | recvbuf_[nAuHeaderOffset + 1]) >> 4);
    nAuHeaderOffset += 2;

    for (int i = 0; i < AU_HEADER_LENGTH; ++i) 
    {
        uint16_t AU_HEADER = ((recvbuf_[nAuHeaderOffset] << 8) | recvbuf_[nAuHeaderOffset + 1]);//之后的2字节是AU_HEADER
        uint32_t nAac = (AU_HEADER >> 3);//其中高13位表示一帧AAC负载的字节长度，低3位无用
        vecAacFrameLen.push_back(nAac);
        nAuHeaderOffset += 2;
    }
    ulRecvLen -= nAuHeaderOffset;
  
    uint8_t* pAacPayload = (uint8_t*)recvbuf_ + nAuHeaderOffset;//真正AAC负载开始处
    uint32_t nAacPayloadOffset = 0;
    auto formatContex = m_conn->get_client_av_format();

    for (int j = 0; j < AU_HEADER_LENGTH; ++j) 
    {
        uint32_t nAac = vecAacFrameLen.at(j);
        //generate ADTS header
        SAacParam param(nAac, formatContex->streams[m_streamId]->codecpar->sample_rate, formatContex->streams[m_streamId]->codecpar->channels);
        CADTS adts;
        adts.init(param);

        /* send direct */
        uint32_t aacLen = adts.getBufSize() + nAac;
        uint32_t bufflen;
        char* recBuf = m_conn->handle_connection_databuf(aacLen, bufflen);
        if (!recBuf) 
        {
            AS_LOG(AS_LOG_ERROR, "fail to handle rtp packet, alloc buffer short.");
            return AS_ERROR_CODE_MEM;
        }

        //write ADTS header
        memcpy(recBuf + nLenOut, adts.getBuf(), adts.getBufSize());
        nLenOut += adts.getBufSize();

        //write AAC payload
        memcpy(recBuf + nLenOut, pAacPayload + nAacPayloadOffset, nAac);
        nLenOut += nAac;
        nAacPayloadOffset += nAac;

        if (nAacPayloadOffset > ulRecvLen) {
            AS_LOG(AS_LOG_WARNING, "fail to handle aac data, invalid len has dectected!");
            break;
        }

        MediaDataInfo dataInfo;
        memset(&dataInfo, 0, sizeof(dataInfo));
        packMediaInfo(dataInfo, rtpPacket.GetTimeStamp(), 0);
        m_conn->handle_connection_media(&dataInfo, aacLen);
    }
    return AS_ERROR_CODE_OK;
}
