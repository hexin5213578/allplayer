#include "hevc.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_rtsp_rtp_frame_organizer.h"
#include "sps_parser.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include "base64.h"

static const uint8_t start_sequence[] = { 0, 0, 0, 1 };

#define H265_TYPE(v) (((uint8_t)(v) >> 1) & 0x3f)

bool getHEVCInfo(const char* vps, size_t vps_len, const char* sps, size_t sps_len, int& iVideoWidth, int& iVideoHeight, float& iVideoFps) {
    T_GetBitContext tGetBitBuf;
    T_HEVCSPS tH265SpsInfo;
    T_HEVCVPS tH265VpsInfo;
    if (vps_len > 2) {
        memset(&tGetBitBuf, 0, sizeof(tGetBitBuf));
        memset(&tH265VpsInfo, 0, sizeof(tH265VpsInfo));
        tGetBitBuf.pu8Buf = (uint8_t*)vps + 2;
        tGetBitBuf.iBufSize = (int)(vps_len - 2);
        if (0 != h265DecVideoParameterSet((void*)&tGetBitBuf, &tH265VpsInfo)) {
            return false;
        }
    }

    if (sps_len > 2) {
        memset(&tGetBitBuf, 0, sizeof(tGetBitBuf));
        memset(&tH265SpsInfo, 0, sizeof(tH265SpsInfo));
        tGetBitBuf.pu8Buf = (uint8_t*)sps + 2;
        tGetBitBuf.iBufSize = (int)(sps_len - 2);
        if (0 != h265DecSeqParameterSet((void*)&tGetBitBuf, &tH265SpsInfo)) {
            return false;
        }
    }
    else {
        return false;
    }
    h265GetWidthHeight(&tH265SpsInfo, &iVideoWidth, &iVideoHeight);
    iVideoFps = 0;
    h265GeFramerate(&tH265VpsInfo, &tH265SpsInfo, &iVideoFps);
    return true;
}

int HEVCTrack::handleRtpPacket(as_msg_block* block)
{
    if (m_conn->get_client_frags() > 1) {
        return m_organizer.insertRtpPacket(block);
    }
    char* buff = block->base();
    auto len = block->length();
    int ret = 0;
    mk_rtp_packet rtp;
    if ((ret = rtp.ParsePacket(buff, len)) || (ret = checkValid(rtp))) {
        return ret;
    }

    int tid, lid, nal_type;
    auto nal_ptr = (uint8_t*)buff + rtp.GetHeadLen();
    nal_type = H265_TYPE(*nal_ptr);
    lid = ((buff[rtp.GetHeadLen()] << 5) & 0x20) | ((buff[rtp.GetHeadLen() + 1] >> 3) & 0x1f);
    tid = buff[rtp.GetHeadLen() + 1] & 0x07;

    if (lid) {
        //AS_LOG(AS_LOG_WARNING, "Multi-layer HEVC coding");
        return -1;
    }

    /* sanity check for correct temporal ID */
    if (!tid) {
        AS_LOG(AS_LOG_WARNING, "Illegal temporal ID in RTP/HEVC packet\n");
        return -1;
    }

    if (nal_type > 50) {
        AS_LOG(AS_LOG_WARNING, "Unsupported (HEVC) NAL type (%d)", nal_type);
        return -1;
    }

    auto rtpHeadLen = rtp.GetHeadLen();
    auto rtpPayloadLen = len - rtpHeadLen - rtp.GetTailLen();
	FU_HEADER* pFu_hdr = nullptr;

    switch (nal_type) {
        /* video parameter set (VPS) */
    case RTP_HEVC_NAL_VPS:
        m_vps = make_shared<ASFrame>();
        m_vps->m_buffer = std::string(buff + rtpHeadLen, rtpPayloadLen);
        m_vps->m_dts = m_vps->m_pts = rtp.GetTimeStamp() / 90;
        break;
        /* sequence parameter set (SPS) */
    case RTP_HEVC_NAL_SPS:
        m_sps = make_shared<ASFrame>();
        m_sps->m_buffer = std::string(buff + rtpHeadLen, rtpPayloadLen);
        m_sps->m_dts = m_sps->m_pts = rtp.GetTimeStamp() / 90;
        break;
        /* picture parameter set (PPS) */
    case RTP_HEVC_NAL_PPS:
        m_pps = make_shared<ASFrame>();
        m_pps->m_buffer = std::string(buff + rtpHeadLen, rtpPayloadLen);
        m_pps->m_dts = m_pps->m_pts = rtp.GetTimeStamp() / 90;
        break;
        /*  supplemental enhancement information (SEI) */
    case RTP_HEVC_NAL_SEI_PREFIX:
        break;
        
    case RTP_H265_NALU_TYPE_FU_A:
		/*if (m_conn->get_client_frags() < -1)*/ {
			pFu_hdr = (FU_HEADER*)&buff[rtp.GetHeadLen() + 2]; //h265 FU_INDICATOR �����ֽ�
			if (pFu_hdr->E) {
				rtp.SetMarker(true);
			}
		}
        ret = m_organizer.insertRtpPacket(block);
        break;
        /* single NAL unit packet */

    default:
        ret = handleRtp((uint8_t*)buff, len);
        break;
    }

    MK_Stream* stream = getTrackStream();
    if (stream && (0 == stream->codecpar->width) && ready()) {
        auto codecpar = stream->codecpar;
        if (getHEVCInfo(m_vps->m_buffer.data(), m_vps->m_buffer.size(), m_sps->m_buffer.data(), m_sps->m_buffer.size(), codecpar->width, codecpar->height, codecpar->video_fps)) {
            m_conn->handle_connection_status(MR_CLIENT_MEDIA_ATTRIBUTE);
        }
        else {
            m_sps.reset();
            m_vps.reset();
            m_pps.reset();
            codecpar->width = 0;
            
        }
    }

    if (RTP_H265_NALU_TYPE_FU_A != nal_type) {
        mk_rtsp_service::instance().free_rtp_recv_buf(block);
        return 0;
    }

    return ret;
}

int32_t HEVCTrack::handleRtp(uint8_t* rtpData, uint32_t rtpLen)
{
    mk_rtp_packet rtpPacket;
    if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket((char*)rtpData, rtpLen)) {
        AS_LOG(AS_LOG_ERROR, "fail to send hevc rtp packet, parse rtp packet fail.");
        return AS_ERROR_CODE_FAIL;
    }

    uint32_t rtpHeadLen = rtpPacket.GetHeadLen();
    uint32_t timeStam = rtpPacket.GetTimeStamp();

    auto rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();
    uint32_t bufflen;

    auto databuf = m_conn->handle_connection_databuf(rtpPayloadLen + 4, bufflen);
    if (!databuf) {
        AS_LOG(AS_LOG_ERROR, "fail handle hevc rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_FAIL;
    }

    int prefix = sizeof(start_sequence);
    memcpy(databuf, start_sequence, prefix);
    uint32_t processIndex = prefix;
    memcpy(&databuf[processIndex], &rtpData[rtpHeadLen], rtpPayloadLen);
    processIndex += rtpPayloadLen;

    MediaDataInfo dataInfo;
    memset(&dataInfo, 0, sizeof(dataInfo));

    auto type = H265_TYPE(databuf[prefix]);
    packMediaInfo(dataInfo, isKeyFrame(type), timeStam, rtpPacket.GetNtp());
    m_conn->handle_connection_media(&dataInfo, processIndex);
    return AS_ERROR_CODE_OK;
}

int32_t HEVCTrack::handleRtpList(RTP_PACK_QUEUE& rtpFrameList)
{
    if (0 == rtpFrameList.size()) {
        return AS_ERROR_CODE_FAIL;
    }

    int32_t nRet = AS_ERROR_CODE_OK;
    if (m_conn->get_client_frags() > 1) {
        nRet = handleMultiFrames(rtpFrameList);
    }
    else  {
        nRet = handleSingleFrame(rtpFrameList);
    }
    return nRet;
}

int HEVCTrack::parse_sdp_a_line() {
    MK_Stream* stream = getTrackStream();
    if (!stream) {
        return AVERROR(EINVAL);
    }

    std::string& fmtp = stream->sdp->strFmtp;
    char attr[256];
    char* value;
    int value_size = fmtp.size() + 1;

    if (!(value = (char*)av_malloc(value_size)))
        return AVERROR(ENOMEM);

    const char* p = fmtp.c_str();
    // remove protocol identifier
    while (*p && *p == ' ')
        p++;                     // strip spaces

    while (fmtp_next_attr_value(&p, attr, sizeof(attr), value, value_size)) {
        auto par = stream->codecpar;

        if (!strcmp(attr, "packetization-mode")) {
            // todo
        }
        else if (!strcmp(attr, "sprop-vps")) {
            m_vps = make_shared<ASFrame>();
            m_vps->m_buffer = decodeBase64(value);
            m_vps->m_dts = m_vps->m_pts = UINT64_MAX;
        }
        else if (!strcmp(attr, "sprop-sps")) {
            m_sps = make_shared<ASFrame>();
            m_sps->m_buffer = decodeBase64(value);
            m_sps->m_dts = m_sps->m_pts = UINT64_MAX;
        }
        else if (!strcmp(attr, "sprop-pps")) {
            m_pps = make_shared<ASFrame>();
            m_pps->m_buffer = decodeBase64(value);
            m_pps->m_dts = m_pps->m_pts = UINT64_MAX;
        }
    }

    if (stream->codecpar && (0 == stream->codecpar->width) && ready()) {
        auto codecpar = stream->codecpar;
        if (getHEVCInfo(m_vps->m_buffer.data(), m_vps->m_buffer.size(), m_sps->m_buffer.data(), m_sps->m_buffer.size(), 
            codecpar->width, codecpar->height, codecpar->video_fps)) {
            m_conn->handle_connection_status(MR_CLIENT_MEDIA_ATTRIBUTE);
           // handleHevcInfoFrame();
        }
        else {
            m_sps.reset();
            m_vps.reset();
            m_pps.reset();
            codecpar->width = 0;
        }
    }

    av_free(value);
    return 0;
}

bool HEVCTrack::ready() {
    return m_vps && m_sps && m_pps;
}

int HEVCTrack::isConfigFrame(int type) {
    if (RTP_HEVC_NAL_VPS  == type || RTP_HEVC_NAL_SPS == type || RTP_HEVC_NAL_PPS == type) {
        return 1;
    }
    return 0;
}

int HEVCTrack::isKeyFrame(int type) {
    if ((type >= RTP_HEVC_NAL_BLA_W_LP) && (type <= RTP_HEVC_NAL_IRAP_VCL23)) {
        return 1;
    }
    return 0;
}

int HEVCTrack::packConfigFrame(uint8_t* data, uint32_t& data_size, uint32_t total_size, uint32_t pts, double ntp) {
    return 0;
}

int32_t HEVCTrack::handleSingleFrame(RTP_PACK_QUEUE& rtpFrameList)
{
    //MK_LOG(AS_LOG_DEBUG, "handleH265Frame rtp packet list:[%d].",rtpFrameList.size());
    if (0 == rtpFrameList.size()) {
        return AS_ERROR_CODE_FAIL;
    }

    mk_rtp_packet rtpPacket;
    uint32_t rtpLen, rtpHeadLen, rtpPayloadLen, timeStam;
    char* rtpData;
    uint32_t      fu_type;
    H265_NALU_HEADER* nalu_hdr = nullptr;

    uint32_t datalen = checkFrameTotalDataLen(rtpFrameList), bufflen;
    char* databuf = m_conn->handle_connection_databuf(datalen, bufflen);
    if (!databuf) {
        AS_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_MEM;
    }
    
    uint32_t m_ulRecvLen = 0;
    if (1 == rtpFrameList.size()) {
        rtpData = rtpFrameList[0].pRtpMsgBlock->base();
        rtpLen = rtpFrameList[0].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(rtpData, rtpLen)) {
            AS_LOG(AS_LOG_ERROR, "handle H265 frame list, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }

        rtpHeadLen = rtpPacket.GetHeadLen();
        timeStam = rtpPacket.GetTimeStamp();
        rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();
        if ((rtpLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen())) {
            AS_LOG(AS_LOG_ERROR, "handle H265 frame list,dataLen invalid, seq[%d], ts[%d], tail[%d]", 
                 rtpPacket.GetSeqNum(), timeStam, rtpPacket.GetTailLen());
            return AS_ERROR_CODE_INVALID;
        }

        nalu_hdr = (H265_NALU_HEADER*)&rtpData[rtpHeadLen];
        
        auto type = (rtpData[rtpHeadLen] & 0x7E) >> 1;
        int prefix = sizeof(start_sequence);
        memcpy(databuf, start_sequence, prefix);
        m_ulRecvLen = prefix;
        //MK_LOG(AS_LOG_DEBUG, "**handle nalu:[%d] rtpHeadLen[%d]pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        memcpy(&databuf[m_ulRecvLen], &rtpData[rtpHeadLen], rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;

        if (ready() && isKeyFrame(type)) {
            packConfigFrame((uint8_t *)databuf, m_ulRecvLen, bufflen, timeStam, rtpPacket.GetNtp());
        }

        MediaDataInfo dataInfo;
        memset(&dataInfo, 0x0, sizeof(dataInfo));
        packMediaInfo(dataInfo, isKeyFrame(type), timeStam, rtpPacket.GetNtp());
        m_conn->handle_connection_media(&dataInfo, m_ulRecvLen);
        return AS_ERROR_CODE_OK;
    }

    for (uint32_t i = 0; i < rtpFrameList.size(); i++) {
        rtpData = rtpFrameList[i].pRtpMsgBlock->base();
        rtpLen = rtpFrameList[i].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(rtpData, rtpLen)) {
            AS_LOG(AS_LOG_ERROR, "handleH265Frame rtp packet, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }
        rtpHeadLen = rtpPacket.GetHeadLen();
        rtpData = &rtpData[rtpHeadLen];
        rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();

        if ((rtpLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen())) {
            AS_LOG(AS_LOG_ERROR, "handle H265 frame list,dataLen invalid.seq[%d], ts[%d], tailLen[%d]",
                rtpPacket.GetSeqNum(), rtpPacket.GetTimeStamp(), rtpPacket.GetTailLen());
            return AS_ERROR_CODE_INVALID;
        }

        FU_HEADER* fu_hdr = (FU_HEADER*)(rtpData + 2);
        if (fu_hdr->S || 0 == i) {
            //fu_type = pData[2] & 0x3f;
            /* first packet */
            memcpy(databuf, start_sequence, sizeof(start_sequence));
            m_ulRecvLen = 4;
            uint8_t nal_unit_type = rtpData[2] & 0x3f;
            databuf[m_ulRecvLen] = (rtpData[0] & 0x81) | (nal_unit_type << 1);
            m_ulRecvLen++;
            databuf[m_ulRecvLen] = rtpData[1];
            m_ulRecvLen++;
            timeStam = rtpPacket.GetTimeStamp();
        }

        if (rtpPayloadLen < 3) {
            AS_LOG(AS_LOG_ERROR, "handleH265Frame rtp packet, DataLen invalid. rtpPayloadLen[%d]", rtpPayloadLen);
            return AS_ERROR_CODE_INVALID;
        }
        rtpData += 3;
        rtpPayloadLen -= 3;
        memcpy(&databuf[m_ulRecvLen], rtpData, rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;

        if (1 == fu_hdr->E || rtpFrameList.size() - 1 == i) {
            MediaDataInfo dataInfo;
            memset(&dataInfo, 0x0, sizeof(dataInfo));
            char ch = databuf[sizeof(start_sequence)];
            int type = (ch & 0x7E) >> 1;

            if (ready() && isKeyFrame(type)) {
                packConfigFrame((uint8_t*)databuf, m_ulRecvLen, bufflen, timeStam, rtpPacket.GetNtp());
            }
            packMediaInfo(dataInfo, isKeyFrame(type), timeStam, rtpPacket.GetNtp());
            m_conn->handle_connection_media(&dataInfo, m_ulRecvLen);
            m_ulRecvLen = 0;
        }
    }
    return AS_ERROR_CODE_OK;
}

int32_t HEVCTrack::handleMultiFrames(RTP_PACK_QUEUE& rtpFrameList)
{
    //MK_LOG(AS_LOG_DEBUG, "handleH265Frame rtp packet list:[%d].",rtpFrameList.size());
    if (0 == rtpFrameList.size() || 1 == rtpFrameList.size()) {
        return AS_ERROR_CODE_FAIL;
    }
    
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen, rtpPayloadLen, dataLen, bufflen;
    char* pData;
    uint32_t      timeStam = 0;
    uint32_t      fu_type;
    H265_NALU_HEADER* nalu_hdr;

    uint32_t ulTotaldatalen = checkFrameTotalDataLen(rtpFrameList);
    char* recBuf = m_conn->handle_connection_databuf(ulTotaldatalen, bufflen);
    if (!recBuf) {
        AS_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_MEM;
    }

    uint32_t m_ulRecvLen = 0;
    uint16_t fragmentIndex = 0;
    uint32_t frag_start = 0;
    uint32_t stream_begin = 0;
    uint16_t start_seq;

    uint16_t frags = m_conn->get_client_frags();
    std::vector<MultiDataInfo> dataInfos;
    dataInfos.reserve(frags);

    for (uint32_t idx = 0; idx < rtpFrameList.size(); idx++) {
        pData = rtpFrameList[idx].pRtpMsgBlock->base();
        dataLen = rtpFrameList[idx].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData, dataLen)) {
            AS_LOG(AS_LOG_ERROR, "handleH265MultiFrame rtp packet, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }
        rtpHeadLen = rtpPacket.GetHeadLen();
        pData = &pData[rtpHeadLen];
        rtpPayloadLen = dataLen - rtpHeadLen - rtpPacket.GetTailLen();

        if ((dataLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen())) {
            AS_LOG(AS_LOG_ERROR, "handleH265Frame rtp packet, DataLen invalid.TailLen[%d]", rtpPacket.GetTailLen());
            return AS_ERROR_CODE_INVALID;
        }

        if (rtpPayloadLen < 3) {
            AS_LOG(AS_LOG_ERROR, "handleH265Frame rtp packet, DataLen invalid, rtpPayloadLen[%d]", rtpPayloadLen);
            return AS_ERROR_CODE_INVALID;
        }

        H265_NALU_HEADER* pNalu_hdr = (H265_NALU_HEADER*)pData;
        if (RTP_H265_NALU_TYPE_FU_A != pNalu_hdr->TYPE) {
            uint32_t cur_frag_start = m_ulRecvLen;
            memcpy(recBuf + m_ulRecvLen, start_sequence, sizeof(start_sequence));
            m_ulRecvLen += sizeof(start_sequence);
            memcpy(recBuf + m_ulRecvLen, pData, rtpPayloadLen);
            m_ulRecvLen += rtpPayloadLen;
            MediaDataInfo dataInfo;
            memset(&dataInfo, 0x0, sizeof(MediaDataInfo));
            dataInfo.offset = cur_frag_start;
            dataInfo.fragment = (fragmentIndex %= frags);
            fragmentIndex++;
            uint32_t rtplen = m_ulRecvLen - cur_frag_start;
            
            char c = recBuf[cur_frag_start + sizeof(start_sequence)];
            int type = (c & 0x7E) >> 1;

            if (ready() && isKeyFrame(type)) {
                packConfigFrame((uint8_t*)recBuf, m_ulRecvLen, bufflen, timeStam, rtpPacket.GetNtp());
            }
            packMediaInfo(dataInfo, isKeyFrame(type), timeStam);
            MultiDataInfo info{ dataInfo, rtplen };
            dataInfos.push_back(info);
            frag_start = m_ulRecvLen;
            continue;
        }

        FU_HEADER* fu_hdr = (FU_HEADER*)(pData + 2);
        if (1 == fu_hdr->S) {
            //fu_type = pData[2] & 0x3f;
            /* first packet */
            stream_begin = idx;
            start_seq = rtpPacket.GetSeqNum();
            memcpy(recBuf, start_sequence, sizeof(start_sequence));
            m_ulRecvLen += sizeof(start_sequence);
            uint8_t nal_unit_type = pData[2] & 0x3f;
            recBuf[m_ulRecvLen] = (pData[0] & 0x81) | (nal_unit_type << 1);
            m_ulRecvLen++;
            recBuf[m_ulRecvLen] = pData[1];
            m_ulRecvLen++;
            timeStam = rtpPacket.GetTimeStamp();
        }
        pData += 3;
        rtpPayloadLen -= 3;

        memcpy(&recBuf[m_ulRecvLen], pData, rtpPayloadLen);
        m_ulRecvLen += rtpPayloadLen;

        if (1 == fu_hdr->E) {
            uint32_t curr_nal_start = frag_start;
            uint32_t curr_find_index = frag_start + 4;
            bool findNext = true;

            do {
                uint32_t i = curr_find_index;
                int headLen = 4;
                while (recBuf[i] != 0x00 || recBuf[i + 1] != 0x00 || recBuf[i + 2] != 0x00 || recBuf[i + 3] != 0x01) {
                    i++;
                    if (i + 4 > m_ulRecvLen) {
                        findNext = false;
                        break;
                    }
                }
                i += 4;

                if (recBuf[curr_nal_start] != 0x00 || recBuf[curr_nal_start + 1] != 0x00 || recBuf[curr_nal_start + 2] != 0x00
                    || recBuf[curr_nal_start + 3] != 0x01) {
                    AS_LOG(AS_LOG_WARNING, "h265 invalid rtp data, parse failed.");
                    return AS_ERROR_CODE_INVALID;
                }
                uint32_t rtplen = findNext ? i - headLen - curr_nal_start : m_ulRecvLen - curr_nal_start;
                MediaDataInfo dataInfo;
                memset(&dataInfo, 0x0, sizeof(dataInfo));
                dataInfo.offset = curr_nal_start;
                dataInfo.fragment = fragmentIndex;

                char c = recBuf[curr_nal_start + sizeof(start_sequence)];
                int type = (c & 0x7E) >> 1;

                if (ready() && isKeyFrame(type)) {
                    packConfigFrame((uint8_t*)recBuf, m_ulRecvLen, bufflen, timeStam, rtpPacket.GetNtp());
                }
                packMediaInfo(dataInfo, isKeyFrame(type), timeStam);
                MultiDataInfo info{ dataInfo, rtplen };
                dataInfos.push_back(info);
                curr_nal_start = i - headLen;
                curr_find_index = i;
            } while (findNext);

            fragmentIndex++;
            frag_start = m_ulRecvLen;

            if (frags > 1 && fragmentIndex > frags) {
                AS_LOG(AS_LOG_WARNING, "h265 parse %d fragment too much, total %d.", fragmentIndex, frags);
                m_conn->handle_connection_status(MR_CLIENT_FRAG_MISTACH);
            }
        }
    }

    if (1 != rtpPacket.GetMarker()) {
        AS_LOG(AS_LOG_WARNING, "not recv marker");
        //break;
    }

    if (fragmentIndex == frags) {
        // MK_LOG(AS_LOG_WARNING, "split %d nb streams total", m_fragmentIndex);
        for (auto data : dataInfos) {
            m_conn->handle_connection_media(&data.info, data.rtplen);
        }
    }
    else {
        AS_LOG(AS_LOG_WARNING, "m_fragmentIndex %d,m_fragements %d, dataInfos.size %d,drop data", fragmentIndex, frags, dataInfos.size());
        return AS_ERROR_CODE_INVALID;
        //handle_connection_status(MR_CLIENT_FRAG_MISTACH);
    }
    return AS_ERROR_CODE_OK;
}
