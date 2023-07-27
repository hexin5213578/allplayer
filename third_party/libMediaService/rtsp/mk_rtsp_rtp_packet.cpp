
#include "as.h"
#include "mk_rtsp_rtp_packet.h"
#include "mk_media_common.h"
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

static const int kHWProfile = 0x4857;
static const int kHWMinExtsion = 20;

mk_rtp_packet::mk_rtp_packet() {
    m_pRtpData      = NULL;
    m_ulPacketLen   = 0;
    m_ulHeadLen     = 0;
    m_ulTailLen     = 0;

    m_pFixedHead    = NULL;
    m_pExtHead      = NULL;
}

mk_rtp_packet::~mk_rtp_packet() {
    m_pRtpData      = NULL;
    m_pFixedHead    = NULL;
    m_pExtHead      = NULL;
}

int32_t mk_rtp_packet::ParsePacket(const char* pRtpData,uint32_t ulLen)
{
    if (NULL == pRtpData) {
        MK_LOG(AS_LOG_WARNING,"mk_rtp_packet::Parse fail, rtp data is null.");
        return AS_ERROR_CODE_PARAM;
    }

    int size = sizeof(RTP_FIXED_HEADER);
    if (ulLen < sizeof(RTP_FIXED_HEADER)) {
        MK_LOG(AS_LOG_WARNING,"mk_rtp_packet::Parse fail, rtp data len is shorter than fixed head len.");
        return AS_ERROR_CODE_PARAM;
    }

    m_pRtpData = (char*)pRtpData;
    m_ulPacketLen = ulLen;
    m_pFixedHead = (RTP_FIXED_HEADER*)(void*)m_pRtpData;

    if(1 == m_pFixedHead->padding) 
    {
        const char* pTail = pRtpData + ulLen -1;

         m_ulTailLen = (uint32_t)(*(uint8_t*)pTail);
        // if(AS_ERROR_CODE_OK != SetPadding(0))
        //      MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, SetPadding fail.");
    }
    uint32_t ulHeadLen = sizeof(RTP_FIXED_HEADER) + m_pFixedHead->csrc_len * RTP_CSRC_LEN;
    if (ulLen < ulHeadLen) {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, rtp data len is shorter than fixed head len.");
        return AS_ERROR_CODE_PARAM;
    }

    if (AS_ERROR_CODE_OK != CheckVersion()) {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, check version fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (1 != m_pFixedHead->extension) {
        m_ulHeadLen = ulHeadLen;
        return AS_ERROR_CODE_OK;
    }

    if (ulLen < ulHeadLen + sizeof(RTP_EXTEND_HEADER)) {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, packet len is too short to contain extend head.");
        return AS_ERROR_CODE_PARAM;
    }

    m_pExtHead = (RTP_EXTEND_HEADER*)(void*)(m_pRtpData + ulHeadLen);
    auto total = ulHeadLen + sizeof(RTP_EXTEND_HEADER) + ntohs(m_pExtHead->usLength) * RTP_EXTEND_PROFILE_LEN;
    if (ulLen < total) {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::Parse fail, packet len is too short.");
        return AS_ERROR_CODE_PARAM;
    }

    if (kHWProfile == ntohs(m_pExtHead->usProfile)) {
        //MK_LOG(AS_LOG_NOTICE, "HW rtp extension profile.");
        auto extlen = ntohs(m_pExtHead->usLength) * RTP_EXTEND_PROFILE_LEN;
        if (extlen < kHWMinExtsion) {
            MK_LOG(AS_LOG_NOTICE, "invliad HW rtp extension profile, too short.");
        }
        
        //TODO 整理代码
        uint8_t* data_ptr = reinterpret_cast<uint8_t*>(m_pRtpData) + ulHeadLen + sizeof(RTP_EXTEND_HEADER);

        uint16_t ms_network_order = *reinterpret_cast<uint16_t*>(data_ptr + 14);
        uint16_t ms = ntohs(ms_network_order);

        uint32_t second_network_order = *reinterpret_cast<uint32_t*>(data_ptr + 16);
        uint32_t second = ntohl(second_network_order);
        m_ntp = second / 1.0 + ms / 1000.0;
    }
    else {
        //MK_LOG(AS_LOG_NOTICE, "unkown rtp extension profile, %u.", ntohs(m_pExtHead->usProfile));
        m_ntp = 0.0;
    }

    m_ulHeadLen = total;
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::GeneratePacket(char* pRtpPacket, uint32_t ulLen, uint8_t channel)
{
    if (NULL == pRtpPacket) {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GeneratePacket fail, packet is null.");
        return AS_ERROR_CODE_PARAM;
    }

    m_pRtpData = pRtpPacket;
    m_ulPacketLen = ulLen;
    m_pRtpData[0] = RTP_INTERLEAVE_FLAG;
    m_pRtpData[1] = channel;
    *(uint16_t*)&m_pRtpData[2] = htons((uint16_t)(ulLen + sizeof(RTP_FIXED_HEADER)));

    m_pFixedHead = (RTP_FIXED_HEADER*)(void*)&m_pRtpData[4];
    m_pFixedHead->version = RTP_PACKET_VERSION;
    m_pFixedHead->marker  = 0;
    m_pFixedHead->payload = 96;
    m_pFixedHead->extension = 0;
    m_ulHeadLen = sizeof(RTP_FIXED_HEADER);
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::GetVersion(char& cVersion)const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetVersion fail, fixed head is null.");
        return AS_ERROR_CODE_FAIL;
    }

    cVersion = m_pFixedHead->version;
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::CheckVersion()const
{
    char cVersion = 0;
    if (AS_ERROR_CODE_OK != GetVersion(cVersion))
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::CheckVersion fail, get packet version fail.");
        return AS_ERROR_CODE_FAIL;
    }

    if (RTP_PACKET_VERSION != cVersion)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::CheckVersion fail, version [%d] is invalid.",
            cVersion);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}

uint16_t mk_rtp_packet::GetSeqNum()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetSeqNum fail, packet is null.");
        return 0;
    }

    return ntohs(m_pFixedHead->seq_no);
}

uint32_t  mk_rtp_packet::GetTimeStamp()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetTimeStamp fail, packet is null.");
        return 0;
    }

    return ntohl(m_pFixedHead->timestamp);
}

uint8_t mk_rtp_packet::GetPayloadType()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetPayloadType fail, packet is null.");
        return 0;
    }

    return m_pFixedHead->payload;
}

bool mk_rtp_packet::GetMarker()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetMarker fail, packet is null.");
        return false;
    }

    return m_pFixedHead->marker;
}

uint32_t mk_rtp_packet::GetSSRC()const
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::GetSSRC fail, packet is null.");
        return 0;
    }

    return m_pFixedHead->ssrc;
}

uint32_t mk_rtp_packet::GetHeadLen()const
{
    return m_ulHeadLen;
}

uint32_t mk_rtp_packet::GetTailLen()const
{
    return m_ulTailLen;
}

int32_t mk_rtp_packet::SetVersion(uint8_t ucVersion)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetVersion fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->version = ucVersion;

    return AS_ERROR_CODE_OK;
}
int32_t mk_rtp_packet::SetPadding(uint8_t ucPadding)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetPadding fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->padding= ucPadding;

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::SetSeqNum(uint16_t usSeqNum)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetSeqNum fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->seq_no = ntohs(usSeqNum);

    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::SetTimeStamp(uint32_t ulTimeStamp)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetTimeStamp fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->timestamp = ntohl(ulTimeStamp);
    return AS_ERROR_CODE_OK;
}

void mk_rtp_packet::SetSSRC(uint32_t unSsrc)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetTimeStamp fail, packet is null.");
        return;
    }

    m_pFixedHead->ssrc = unSsrc;
    return;
}

int32_t mk_rtp_packet::SetPayloadType(uint8_t ucPayloadType)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetPayloadType fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->payload = ucPayloadType;
    return AS_ERROR_CODE_OK;
}

int32_t mk_rtp_packet::SetMarker(bool bMarker)
{
    if (NULL == m_pFixedHead)
    {
        MK_LOG(AS_LOG_ERROR,"mk_rtp_packet::SetMarker fail, packet is null.");
        return AS_ERROR_CODE_FAIL;
    }

    m_pFixedHead->marker = bMarker;

    return AS_ERROR_CODE_OK;
}

uint16_t mk_rtp_packet::GetSeqNum(const char* pMb)
{

    if (NULL == pMb)
    {
        MK_LOG(AS_LOG_ERROR, "mk_rtp_packet::GetSeqNum fail, mb is null.");
        return 0;
    }

    RTP_FIXED_HEADER* FixedHead = (RTP_FIXED_HEADER*)(void*)pMb;

    return ntohs(FixedHead->seq_no);
}

