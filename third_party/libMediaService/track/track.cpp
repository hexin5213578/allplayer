#include "track.h"
#include "mk_rtsp_rtp_packet.h"

string FindField(const char* buf, const char* start, const char* end, size_t bufSize) {
    if (bufSize <= 0) {
        bufSize = strlen(buf);
    }
    const char* msg_start = buf, * msg_end = buf + bufSize;
    size_t len = 0;
    if (start != NULL) {
        len = strlen(start);
        msg_start = strstr(buf, start);
    }
    if (msg_start == NULL) {
        return "";
    }
    msg_start += len;
    if (end != NULL) {
        msg_end = strstr(msg_start, end);
        if (msg_end == NULL) {
            return "";
        }
    }
    return string(msg_start, msg_end);
}

#define SPACE_CHARS " \t\r\n"

static void get_word_until_chars(char* buf, int buf_size, const char* sep, const char** pp) {
    const char* p;
    char* q;

    p = *pp;
    p += strspn(p, SPACE_CHARS);
    q = buf;
    while (!strchr(sep, *p) && *p != '\0') {
        if ((q - buf) < buf_size - 1)
            *q++ = *p;
        p++;
    }
    if (buf_size > 0)
        *q = '\0';
    *pp = p;
}

static void get_word_sep(char* buf, int buf_size, const char* sep, const char** pp) {
    if (**pp == '/') (*pp)++;
    get_word_until_chars(buf, buf_size, sep, pp);
}

int fmtp_next_attr_value(const char** p, char* attr, int attr_size, char* value, int value_size) {
    *p += strspn(*p, SPACE_CHARS);
    if (**p) {
        get_word_sep(attr, attr_size, "=", p);
        if (**p == '=')
            (*p)++;
        get_word_sep(value, value_size, ";", p);
        if (**p == ';')
            (*p)++;
        return 1;
    }
    return 0;
}

//===========================================================

int Track::init() {
    int ret = 0;
    if (ret = m_organizer.init(this, 1 != m_conn->get_client_frags())) {
        return ret;
    }
    return parse_sdp_a_line();
}

void Track::appendRtpStatistics(RTP_PACKET_STAT_INFO& statistic)
{
    uint32_t total, lostp, lostf, disorder;
    m_organizer.getRtpPacketStatInfo(total, lostp, lostf, disorder);
    statistic.ulTotalPackNum += total;
    statistic.ulLostRtpPacketNum += lostp;
    statistic.ulLostFrameNum += lostf;
    statistic.ulDisorderSeqCounts += disorder;
}

int Track::handleRtpFrame(uint8_t payloadType, RTP_PACK_QUEUE& rtpFrameList)
{
    m_organizer.updateTotalRtpPacketNum();
    uint8_t pt = m_conn->get_client_av_format()->streams[m_streamId]->payload;
    if (payloadType != pt) {
        AS_LOG(AS_LOG_WARNING, "not support payload %d in stream %d.", payloadType, m_streamId);
        return AS_ERROR_CODE_INVALID;
    }

    return handleRtpList(rtpFrameList);
}

int Track::checkValid(mk_rtp_packet& rtp)
{
    int ret = 0;
    auto rtp_payload = rtp.GetPayloadType();
    auto stream_payload = m_conn->get_client_av_format()->streams[m_streamId]->payload;
    if (rtp_payload != stream_payload) {
        AS_LOG(AS_LOG_WARNING, "payload type %d is not fit in stream %d.", rtp_payload, m_streamId);
        ret = AS_ERROR_CODE_INVALID;
    }
    return ret;
}

static int64_t rescaleTb(uint32_t pts, MK_Rational inTb, MK_Rational outTb) {
    double inSec = pts * (double)inTb.num / (double)inTb.den;
    int64_t outPts = inSec * outTb.den / outTb.num;
    return outPts;
}

int Track::packMediaInfo(MediaDataInfo& dataInfo, int key, uint32_t pts, double ntp) {
    auto stream = getTrackStream();
    if (!stream) {
        return AS_ERROR_CODE_INVALID;
    }

    MK_Rational outTb = { 1,1000 };  //Êä³ötimebase ºÁÃë
    int64_t msPts = 0;

    msPts = rescaleTb(pts, stream->time_base, outTb); //for calcu timestamp
    dataInfo.codec_id = stream->codecpar->codec_id;
    dataInfo.code = MR_MEDIA_CODE_OK;
    dataInfo.is_key = key;
    dataInfo.pts = msPts;
    dataInfo.ntp = ntp;
    dataInfo.stream_index = m_streamId;
    return 0;
}

int32_t Track::checkFrameTotalDataLen(RTP_PACK_QUEUE& rtpFrameList)
{
    mk_rtp_packet   rtpPacket;
    uint32_t        rtpHeadLen, rtpPayloadLen = 0;
    char* pData = nullptr;
    uint32_t      dataLen;
    for (uint32_t i = 0; i < rtpFrameList.size(); i++) {
        pData = rtpFrameList[i].pRtpMsgBlock->base();
        dataLen = rtpFrameList[i].pRtpMsgBlock->length();
        if (AS_ERROR_CODE_OK != rtpPacket.ParsePacket(pData, dataLen)) {
            AS_LOG(AS_LOG_ERROR, "check frame length, parse rtp packet fail.");
            return 0;
        }
        rtpHeadLen = rtpPacket.GetHeadLen();
        uint32_t ulen = dataLen - rtpHeadLen - rtpPacket.GetTailLen();
        rtpPayloadLen += ulen;
    }
    return rtpPayloadLen;
}

MK_Stream* Track::getTrackStream() {
    MK_Stream* stream = nullptr;
    MK_Format_Contex* fctx = m_conn->get_client_av_format();
    if (m_streamId >=0 && m_streamId < fctx->streams.size()) {
        stream = fctx->streams.at(m_streamId);
    }
    return stream;
}
