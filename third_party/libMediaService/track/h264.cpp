#include "h264.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_rtsp_connection.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include "base64.h"
#include "sps_parser.h"

static bool getAVCInfo(uint8_t* sps, size_t sps_len, int& iVideoWidth, int& iVideoHeight, float& iVideoFps) {
    if (sps_len < 4) {
        return false;
    }
    T_GetBitContext tGetBitBuf;
    T_SPS tH264SpsInfo;
    memset(&tGetBitBuf, 0, sizeof(tGetBitBuf));
    memset(&tH264SpsInfo, 0, sizeof(tH264SpsInfo));
    tGetBitBuf.pu8Buf = sps + 1;
    tGetBitBuf.iBufSize = (int)(sps_len - 1);
    if (0 != h264DecSeqParameterSet((void*)&tGetBitBuf, &tH264SpsInfo)) {
        return false;
    }
    h264GetWidthHeight(&tH264SpsInfo, &iVideoWidth, &iVideoHeight);
    h264GeFramerate(&tH264SpsInfo, &iVideoFps);
    return true;
}

struct PayloadContext {
    // sdp setup parameters
    uint8_t profile_idc;
    uint8_t profile_iop;
    uint8_t level_idc;
    int packetization_mode;
};

static const uint8_t start_sequence[] = { 0, 0, 0, 1 };

H264Track::~H264Track()
{
    if (MK_Stream* stream = getTrackStream()) {
        auto par = stream->codecpar;
        par->extradata_size = 0;
        av_freep(&par->extradata);
    }
}

int H264Track::parse_sdp_a_line()
{
    MK_Stream* stream = getTrackStream();
    if (!stream) {
        return AVERROR(EINVAL);
    }

    std::string& fmtp = stream->sdp->strFmtp;
    char attr[256];
    char* value;
    int res = 0;
    int value_size = fmtp.size() + 1;

    if (!(value = (char*)av_malloc(value_size)))
        return AVERROR(ENOMEM);

    const char* p = fmtp.c_str();
    // remove protocol identifier
    while (*p && *p == ' ')
        p++;                     // strip spaces
    while (*p && *p != ' ')
        p++;                     // eat protocol identifier
    while (*p && *p == ' ')
        p++;                     // strip trailing spaces

    while (fmtp_next_attr_value(&p, attr, sizeof(attr), value, value_size)) {
        auto par = stream->codecpar;
       
        if (!strcmp(attr, "packetization-mode")) {
            // todo
        }
        else if (!strcmp(attr, "sprop-parameter-sets")) {
            if (*value == 0 || value[strlen(value) - 1] == ',') {
                AS_LOG(AS_LOG_WARNING, "Missing PPS in sprop-parameter-sets, ignoring\n");
                continue;
            }

            string base64_SPS = FindField(value, NULL, ",");
			string base64_PPS = FindField(value, ",", NULL);
          
            m_sps = make_shared<ASFrame>();
            m_sps->m_dts = m_sps->m_pts = UINT64_MAX;
            m_sps->m_buffer = decodeBase64(base64_SPS);

            m_pps = make_shared<ASFrame>();
            m_pps->m_dts = m_pps->m_pts = UINT64_MAX;
            m_pps->m_buffer = decodeBase64(base64_PPS);

            if (stream->codecpar->width == 0 && ready()) {
                if (getAVCInfo((uint8_t*)m_sps->m_buffer.data(), m_sps->m_buffer.length(), 
                    stream->codecpar->width, stream->codecpar->height, stream->codecpar->video_fps)) {
                    m_conn->handle_connection_status(MR_CLIENT_MEDIA_ATTRIBUTE);
                }
                else {
                    m_sps.reset();
                    m_pps.reset();
                    stream->codecpar->width = 0;
                }
            }

            par->extradata_size = 0;
            av_freep(&par->extradata);
            res = ff_h264_parse_sprop_parameter_sets(&par->extradata, &par->extradata_size, value);
            if (res) {
                av_free(value);
                return res;
            }
        }
    }
    av_free(value);
    return res;
}

int H264Track::handleRtpPacket(as_msg_block* block)
{
    if (m_conn->get_client_frags() > 1)
        return m_organizer.insertRtpPacket(block);

    FU_HEADER* pFu_hdr = nullptr;
    char* buff = block->base();
    auto len = block->length();

    int ret = 0;
    mk_rtp_packet rtp;
    if ((ret = rtp.ParsePacket(buff, len)) || (ret = checkValid(rtp)))
        return ret;

    pFu_hdr = (FU_HEADER*)&buff[rtp.GetHeadLen()];
    uint8_t nal = buff[rtp.GetHeadLen()];
    uint8_t type = nal & 0x1f;

    if (isConfigFrame(type)) {
        auto head = rtp.GetHeadLen();
        auto payload_len = len - head - rtp.GetTailLen();

        if (RTP_H264_NALU_TYPE_SPS == type) {
            m_sps = make_shared<ASFrame>();
            m_sps->m_buffer = std::string(buff + head, payload_len);
            m_sps->m_dts = m_sps->m_pts = rtp.GetTimeStamp() / 90;
        }
        else if (RTP_H264_NALU_TYPE_PPS == type) {
            m_pps = make_shared<ASFrame>();
            m_pps->m_buffer = std::string(buff + head, payload_len);
            m_pps->m_dts = m_pps->m_pts = rtp.GetTimeStamp() / 90;
        }

        MK_Stream* stream = getTrackStream();
        if (ready() && stream && stream->codecpar->width == 0) {
            if (getAVCInfo((uint8_t*)m_sps->m_buffer.data(), m_sps->m_buffer.length(), 
                stream->codecpar->width, stream->codecpar->height, stream->codecpar->video_fps)) {
                m_conn->handle_connection_status(MR_CLIENT_MEDIA_ATTRIBUTE);
            }
            else {
                m_sps.reset();
                m_pps.reset();
                stream->codecpar->width = 0;
            }
        }

        mk_rtsp_service::instance().free_rtp_recv_buf(block);
        return 0;
    }

    if (type >= 1 && type <= 23) {
        type = 1;
    }

    switch (type) {
    case 0:
    case 1:
        ret = handleRtp((uint8_t*)buff, len);
        break;
    case 24:                   // STAP-A (one packet, multiple nals):
        ret = unpackStapA((uint8_t*)buff, len);
        break;
    case 25:                   // STAP-B
    case 26:                   // MTAP-16
    case 27:                   // MTAP-24
    case 29:                   // FU-B
        AS_LOG(AS_LOG_WARNING, "todo: handle %d packet", type);
        break;
    case RTP_H264_NALU_TYPE_FU_A:
        if(m_conn->get_client_frags() < -1) {
            pFu_hdr = (FU_HEADER*)&buff[rtp.GetHeadLen() + 1];
            if (pFu_hdr->E) {
                rtp.SetMarker(true);
            }
        }
        ret = m_organizer.insertRtpPacket(block);
        break;
    default:
        break;
    }
    
    if (RTP_H264_NALU_TYPE_FU_A != type) {
        mk_rtsp_service::instance().free_rtp_recv_buf(block);
        return 0;
    }
    return ret;
}

int32_t H264Track::handleRtp(uint8_t* rtpData, uint32_t rtpLen)
{
    mk_rtp_packet rtpPacket;
    int ret = 0;
    if (ret = rtpPacket.ParsePacket((char*)rtpData, rtpLen)) {
        return ret;
    }

    uint32_t rtpHeadLen = rtpPacket.GetHeadLen();
    uint32_t timeStam = rtpPacket.GetTimeStamp();
    uint32_t rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();
    uint32_t bufflen = 0;
    int prefixSize = sizeof(start_sequence);

    char* databuf = m_conn->handle_connection_databuf(rtpPayloadLen + 4, bufflen);
    if (!databuf) {
        AS_LOG(AS_LOG_ERROR, "fail handle h264 rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_FAIL;
    }

    memcpy(databuf, start_sequence, prefixSize);
    uint32_t processIndex = prefixSize;
    memcpy(&databuf[processIndex], &rtpData[rtpHeadLen], rtpPayloadLen);
    processIndex += rtpPayloadLen;

    MediaDataInfo data;
    memset(&data, 0, sizeof(data));

    uint8_t nal = databuf[prefixSize];
    uint8_t type = (nal & 0x1f);

    if (m_conn->get_client_frags() < -1) {
        if (timeStam != m_timeStamp) {
            m_timeStamp = timeStam;
            m_currFrag = 0;
        }
        else {
            if ((RTP_H264_NALU_TYPE_SPS == type) || RTP_H264_NALU_TYPE_NON_IDR == type)
                ++m_currFrag;
        }

        AS_LOG(AS_LOG_INFO, "h264 timestamp %d, nal_type %d, frag %d.", m_timeStamp, type, m_currFrag);
        data.fragment = m_currFrag;
    }

    if (ready() && isKeyFrame(type)) {
        packConfigFrame((uint8_t*)databuf, processIndex, bufflen, timeStam, rtpPacket.GetNtp());
    }

    packMediaInfo(data, isKeyFrame(type), timeStam, rtpPacket.GetNtp());
    m_conn->handle_connection_media(&data, processIndex);
    return AS_ERROR_CODE_OK;
}

int32_t H264Track::handleRtpList(RTP_PACK_QUEUE& rtpFrameList) {
    if (0 == rtpFrameList.size()) {
        return AS_ERROR_CODE_FAIL;
    }
    
    int32_t nRet = m_conn->get_client_frags() > 1 ? handleMultiFrames(rtpFrameList) : handleSingleFrame(rtpFrameList);
    return nRet;
}

int H264Track::packConfigFrame(uint8_t* data, uint32_t& data_size, uint32_t total_size, uint32_t pts, double ntp) {
    return 0;
}

int H264Track::isConfigFrame(int type) {
    if (RTP_H264_NALU_TYPE_SPS == type || RTP_H264_NALU_TYPE_PPS == type) {
        return 1;
    }
    return 0;
}

int H264Track::isKeyFrame(int type) {
    if (RTP_H264_NALU_TYPE_IDR == type) {
        return 1;
    }
    return 0;
}

int H264Track::unpackStapA(uint8_t* rtpData, uint32_t rtpLen)
{
    mk_rtp_packet rtpPacket;
    int ret = 0;
    if (ret = rtpPacket.ParsePacket((char*)rtpData, rtpLen))
        return ret;

    auto head_len = rtpPacket.GetHeadLen();
    auto timeStam = rtpPacket.GetTimeStamp();
    uint32_t pload_len = rtpLen - head_len - rtpPacket.GetTailLen();
   
    uint8_t* ptr = rtpData + head_len + 1;
    auto end = ptr + pload_len -1;

    int pass = 0;
    int total_length = 0;
    uint8_t* dst = nullptr;

    for (pass = 0; pass < 2; ++pass) {
        const uint8_t* src = ptr;
        int src_len = pload_len -1;

        while (src_len > 2) {
            uint16_t nal_size = (src[0] << 8) | src[1];
            src += 2;
            src_len -= 2;

            if (!nal_size || nal_size > src_len) {
                AS_LOG(AS_LOG_WARNING, "exceeds nal size: %d.", nal_size);
                return AVERROR_INVALIDDATA;
            }
           
            if (pass == 0) {
                // counting
                total_length += sizeof(start_sequence) + nal_size;
            }
            else {
                // copying
                memcpy(dst, start_sequence, sizeof(start_sequence));
                dst += sizeof(start_sequence);
                memcpy(dst, src, nal_size);
                dst += nal_size;
            }
            // eat what we handled
            src += nal_size;
            src_len -= nal_size;
        }

        if (pass == 0) {
            uint32_t bufflen;
            dst = (uint8_t*)m_conn->handle_connection_databuf(total_length, bufflen);
            if (!dst) {
                AS_LOG(AS_LOG_ERROR, "fail handle h264 rtp packet, alloc buffer short.");
                return AS_ERROR_CODE_FAIL;
            }
        }
    }

    MediaDataInfo data;
    memset(&data, 0, sizeof(data));
    packMediaInfo(data, 0, timeStam, rtpPacket.GetNtp());
    m_conn->handle_connection_media(&data, total_length);
    return ret;
}

int32_t H264Track::handleSingleFrame(RTP_PACK_QUEUE& rtpFrameList)
{
    //AS_LOG(AS_LOG_DEBUG, "handleH264Frame rtp packet list:[%d].",rtpFrameList.size());
    mk_rtp_packet rtpPacket;
    uint32_t      rtpLen, rtpHeadLen, rtpPayloadLen, timeStam;
    char* rtpData;
    H264_NALU_HEADER* nalu_hdr = nullptr;
    FU_INDICATOR* fu_ind = nullptr;
    FU_HEADER* fu_hdr = nullptr;

    uint32_t bufflen, ulTotaldatalen = checkFrameTotalDataLen(rtpFrameList);
    char* recBuf = m_conn->handle_connection_databuf(ulTotaldatalen, bufflen);
    if (!recBuf) {
        AS_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_MEM;
    }
   
    uint32_t processIndex = 0;
    int prefixSize = sizeof(start_sequence);
    if (1 == rtpFrameList.size()) {
        rtpData = rtpFrameList[0].pRtpMsgBlock->base();
        rtpLen = rtpFrameList[0].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(rtpData, rtpLen)) {
            AS_LOG(AS_LOG_ERROR, "handleOneH264Frame rtp list, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }

        rtpHeadLen = rtpPacket.GetHeadLen();
        timeStam = rtpPacket.GetTimeStamp();
        rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();
        if ((rtpLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen())) {
            AS_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, dataLen invalid.seq[%d],ts[%d],tail[%d]", 
                 rtpPacket.GetSeqNum(), timeStam, rtpPacket.GetTailLen());
            return AS_ERROR_CODE_INVALID;
        }

        nalu_hdr = (H264_NALU_HEADER*)&rtpData[rtpHeadLen];
        //MK_LOG(AS_LOG_DEBUG, "**handle nalu:[%d] rtpHeadLen[%d]pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        memcpy(recBuf, start_sequence, prefixSize);
        processIndex = prefixSize;
        memcpy(&recBuf[processIndex], &rtpData[rtpHeadLen], rtpPayloadLen);
        processIndex += rtpPayloadLen;

        MediaDataInfo dataInfo;
        memset(&dataInfo, 0x0, sizeof(dataInfo));
        char c = recBuf[prefixSize];
        int type = (c & 0x1f);

        if (m_conn->get_client_frags() < -1) {
            if (timeStam != m_timeStamp) {
                m_timeStamp = timeStam;
                m_currFrag = 0;
            }
            else {
                if (RTP_H264_NALU_TYPE_IDR != type) {
                    ++m_currFrag;
                }
            }
            dataInfo.fragment = m_currFrag;
        }

        if (ready() && isKeyFrame(type)) {
            packConfigFrame((uint8_t*)recBuf, processIndex, bufflen, timeStam, rtpPacket.GetNtp());
        }

        packMediaInfo(dataInfo, isKeyFrame(type), timeStam, rtpPacket.GetNtp());
        m_conn->handle_connection_media(&dataInfo, processIndex);
        return AS_ERROR_CODE_OK;
    }

    for (uint32_t i = 0; i < rtpFrameList.size(); i++) {
        rtpData = rtpFrameList[i].pRtpMsgBlock->base();
        rtpLen = rtpFrameList[i].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(rtpData, rtpLen)) {
            AS_LOG(AS_LOG_ERROR, "handleOneH264Frame frame list, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }
        rtpHeadLen = rtpPacket.GetHeadLen();
        timeStam = rtpPacket.GetTimeStamp();
        rtpPayloadLen = rtpLen - rtpHeadLen - rtpPacket.GetTailLen();

        if ((rtpLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen())) {
            AS_LOG(AS_LOG_ERROR, "handleH264Frame rtp packet, dataLen invalid, seq[%d],ts[%d],tail[%d]",
                rtpPacket.GetSeqNum(), timeStam, rtpPacket.GetTailLen());
            return AS_ERROR_CODE_INVALID;
        }

        rtpData = &rtpData[rtpHeadLen];
        fu_ind = (FU_INDICATOR*)rtpData;  rtpData++;
        fu_hdr = (FU_HEADER*)rtpData;     rtpData++;
        rtpPayloadLen -= 2;
        if (0 == i || fu_hdr->S) {
            /* first packet */
            memcpy(recBuf, start_sequence, sizeof(start_sequence));
            processIndex = prefixSize;
            nalu_hdr = (H264_NALU_HEADER*)&recBuf[processIndex];
            nalu_hdr->TYPE = fu_hdr->TYPE;
            nalu_hdr->F = fu_ind->F;
            nalu_hdr->NRI = fu_ind->NRI;
            processIndex++; /* 1 byte */
            timeStam = rtpPacket.GetTimeStamp();
            //MK_LOG(AS_LOG_DEBUG, "##handle nalu:[%d],rtpHeadLen[%d] pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        }
        memcpy(&recBuf[processIndex], rtpData, rtpPayloadLen);
        processIndex += rtpPayloadLen;

        if (1 == fu_hdr->E || rtpFrameList.size() - 1 == i) {
            MediaDataInfo dataInfo;
            memset(&dataInfo, 0x0, sizeof(dataInfo));
            char c = recBuf[prefixSize];
            int nalType = (c & 0x1f);

            if (m_conn->get_client_frags() < -1) {
                if (timeStam != m_timeStamp) {
                    m_timeStamp = timeStam;
                    m_currFrag = 0;
                }
                else {
                    if (RTP_H264_NALU_TYPE_IDR != nalType) {
                        ++m_currFrag;
                    }
                }
                dataInfo.fragment = m_currFrag;
            }

            if (ready() && isKeyFrame(nalType)) {
                packConfigFrame((uint8_t*)recBuf, processIndex, bufflen, timeStam, rtpPacket.GetNtp());
            }

            packMediaInfo(dataInfo, isKeyFrame(nalType), timeStam, rtpPacket.GetNtp());
            m_conn->handle_connection_media(&dataInfo, processIndex);
            processIndex = 0;
        }
    }
    return AS_ERROR_CODE_OK;
}

int32_t H264Track::handleMultiFrames(RTP_PACK_QUEUE& rtpFrameList)
{
    mk_rtp_packet rtpPacket;
    uint32_t      rtpHeadLen;
    uint32_t      rtpPayloadLen;
    char* pData;
    uint32_t      DataLen;
    uint32_t      timeStam = 0;
    H264_NALU_HEADER* nalu_hdr = NULL;
    FU_INDICATOR* fu_ind = NULL;
    FU_HEADER* fu_hdr = NULL;

    uint16_t frags = m_conn->get_client_frags();
    if (rtpFrameList.size() <= 1) {
        AS_LOG(AS_LOG_WARNING, "one or zero rtp packet can't fit %d multi fragments", frags);
        return AS_ERROR_CODE_QUE_EMPTY;
    }

    uint32_t ulTotaldatalen = checkFrameTotalDataLen(rtpFrameList);
    uint32_t  bufflen;
    char* recBuf = m_conn->handle_connection_databuf(ulTotaldatalen, bufflen);
    if (NULL == recBuf) {
        AS_LOG(AS_LOG_ERROR, "fail to insert rtp packet, alloc buffer short.");
        return AS_ERROR_CODE_MEM;
    }

    uint32_t processIndex = 0;
    uint16_t fragIndex = 0;
    uint32_t fragStart = 0;
    uint32_t streamStart = 0;
    uint16_t startSeq;
    int prefixSize = sizeof(start_sequence);

    std::vector<MultiDataInfo> dataInfos;
    dataInfos.reserve(frags);

    for (uint32_t idx = 0; idx < rtpFrameList.size(); idx++) {
        pData = rtpFrameList[idx].pRtpMsgBlock->base();
        DataLen = rtpFrameList[idx].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData, DataLen)) {
            AS_LOG(AS_LOG_ERROR, "handleH264MultiFrame rtp packet, parse rtp packet fail.");
            return AS_ERROR_CODE_INVALID;
        }
        rtpHeadLen = rtpPacket.GetHeadLen();
        timeStam = rtpPacket.GetTimeStamp();
        rtpPayloadLen = DataLen - rtpHeadLen - rtpPacket.GetTailLen();

        if ((DataLen - 2) < (rtpHeadLen + rtpPacket.GetTailLen())) {
            AS_LOG(AS_LOG_ERROR, "handleH264MultiFrame rtp packet, DataLen invalid.TailLen[%d]", rtpPacket.GetTailLen());
            return AS_ERROR_CODE_INVALID;
        }

        pData = &pData[rtpHeadLen];
        fu_ind = (FU_INDICATOR*)pData;

        if (RTP_H264_NALU_TYPE_FU_A != fu_ind->TYPE) {
            uint32_t cur_frag_start = processIndex;
            memcpy(recBuf + processIndex, start_sequence, prefixSize);
            processIndex += prefixSize;
            memcpy(&recBuf[processIndex], pData, rtpPayloadLen);
            processIndex += rtpPayloadLen;
            MediaDataInfo dataInfo;
            memset(&dataInfo, 0x0, sizeof(MediaDataInfo));
            dataInfo.offset = cur_frag_start;
            dataInfo.fragment = (fragIndex %= frags);
            fragIndex++;

            //MK_LOG(AS_LOG_WARNING, "rtp seq [%d]'s fu_indicator type is not fu_a, parse independently", rtpPacket.GetSeqNum());
            uint32_t rtplen = processIndex - cur_frag_start;
            char c = recBuf[cur_frag_start + prefixSize];
            int nalType = (c & 0x1f);
            if (ready() && isKeyFrame(nalType)) {
                packConfigFrame((uint8_t*)recBuf, processIndex, bufflen, timeStam, rtpPacket.GetNtp());
            }
            packMediaInfo(dataInfo, isKeyFrame(nalType), timeStam);
            MultiDataInfo info{ dataInfo, rtplen };
            dataInfos.push_back(info);
            fragStart = processIndex;
            continue;
        }

        pData++;
        fu_hdr = (FU_HEADER*)pData; pData++;
        rtpPayloadLen -= 2;

        if (1 == fu_hdr->S) {
            streamStart = idx;
            startSeq = rtpPacket.GetSeqNum();

            /* first packet */
            memcpy(recBuf + processIndex, start_sequence, prefixSize);
            processIndex += prefixSize;
            nalu_hdr = (H264_NALU_HEADER*)&recBuf[processIndex];
            nalu_hdr->TYPE = fu_hdr->TYPE;
            nalu_hdr->F = fu_ind->F;
            nalu_hdr->NRI = fu_ind->NRI;
            processIndex++; /* 1 byte */
            timeStam = rtpPacket.GetTimeStamp();
            //MK_LOG(AS_LOG_DEBUG, "##handle nalu:[%d],rtpHeadLen[%d] pt:[%d].",nalu_hdr->TYPE,rtpHeadLen,rtpPacket.GetPayloadType());
        }
        memcpy(&recBuf[processIndex], pData, rtpPayloadLen);
        //MK_LOG(AS_LOG_WARNING,"memcpy %d payload len to recBuf", rtpPayloadLen);
        processIndex += rtpPayloadLen;

        if (1 == fu_hdr->E) {
            char c = recBuf[fragStart + prefixSize];
            int nalType = (c & 0x1f);
            bool findNext = true;

            uint32_t curr_nal_start = fragStart;
            uint32_t curr_find_index = fragStart + prefixSize;
            do {
                uint32_t i = curr_find_index;
                int headLen = prefixSize;

                while (recBuf[i] != 0x00 || recBuf[i + 1] != 0x00 || recBuf[i + 2] != 0x00 || recBuf[i + 3] != 0x01) {
                    i++;
                    if (i + prefixSize > processIndex) {
                        findNext = false;
                        break;
                    }
                }
                i += 4;

                if (recBuf[curr_nal_start] != 0x00 || recBuf[curr_nal_start + 1] != 0x00 ||
                    recBuf[curr_nal_start + 2] != 0x00 || recBuf[curr_nal_start + 3] != 0x01) {
                    AS_LOG(AS_LOG_WARNING, "invalid rtp data, h264 parse failed.");
                    return AS_ERROR_CODE_INVALID;
                }

                uint32_t rtplen = findNext ? i - headLen - curr_nal_start : processIndex - curr_nal_start;

                MediaDataInfo dataInfo;
                memset(&dataInfo, 0x0, sizeof(dataInfo));
                dataInfo.offset = curr_nal_start;
                dataInfo.fragment = fragIndex;

                if (ready() && isKeyFrame(nalType)) {
                    packConfigFrame((uint8_t*)recBuf, processIndex, bufflen, timeStam, rtpPacket.GetNtp());
                }
                packMediaInfo(dataInfo,  isKeyFrame(nalType), timeStam);
                MultiDataInfo info{ dataInfo, rtplen };
                dataInfos.push_back(info);
                curr_nal_start = i - headLen;
                curr_find_index = i;
            } while (findNext);

            fragIndex++;
            if (frags > 1 && fragIndex > frags) {
                AS_LOG(AS_LOG_WARNING, "h264 parse %d fragment too much, total %d.", fragIndex, frags);
                m_conn->handle_connection_status(MR_CLIENT_FRAG_MISTACH);
            }
            fragStart = processIndex;
            //MK_LOG(AS_LOG_WARNING, "split %drd stream from %d packet to %d packet. total %d", m_fragmentIndex, start_seq, rtpPacket.GetSeqNum(), idx - stream_begin + 1);
        }
    }

    if (1 != rtpPacket.GetMarker()) {
        AS_LOG(AS_LOG_WARNING, "h264 frame not recv marker");
    }

    if (fragIndex == frags) {
        //MK_LOG(AS_LOG_WARNING, "split %d nb streams total", m_fragmentIndex);
        for (auto data : dataInfos) {
            m_conn->handle_connection_media(&data.info, data.rtplen);
        }
    }
    else {
        //handle_connection_status(MR_CLIENT_FRAG_MISTACH);
        AS_LOG(AS_LOG_WARNING, "fragment index %d, fragements %d, dataInfos.size %d,drop data", fragIndex, frags, dataInfos.size());
        return AS_ERROR_CODE_INVALID;
    }
    return AS_ERROR_CODE_OK;
}

int H264Track::ff_h264_parse_sprop_parameter_sets(uint8_t** data_ptr, int* size_ptr, const char* value)
{
    char base64packet[1024];
    uint8_t decoded_packet[1024];
    int packet_size;

    while (*value) {
        char* dst = base64packet;

        while (*value && *value != ',' && (dst - base64packet) < sizeof(base64packet) - 1) {
            *dst++ = *value++;
        }
        *dst++ = '\0';

        if (*value == ',')
            value++;

        packet_size = av_base64_decode(decoded_packet, base64packet, sizeof(decoded_packet));
        if (packet_size > 0) {
            uint8_t* dest = (uint8_t*)av_realloc(*data_ptr, packet_size + sizeof(start_sequence) + *size_ptr + AV_INPUT_BUFFER_PADDING_SIZE);
            if (!dest) {
                return AVERROR(ENOMEM);
            }
            *data_ptr = dest;

            memcpy(dest + *size_ptr, start_sequence, sizeof(start_sequence));
            memcpy(dest + *size_ptr + sizeof(start_sequence), decoded_packet, packet_size);
            memset(dest + *size_ptr + sizeof(start_sequence) + packet_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
            *size_ptr += sizeof(start_sequence) + packet_size;
        }
    }

    return 0;
}
