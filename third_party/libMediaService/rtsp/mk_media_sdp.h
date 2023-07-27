#ifndef __MK_RTSP_SDP_INCLUDE___
#define __MK_RTSP_SDP_INCLUDE___

#include <list>
#include "libMediaService.h"

#define STR_SDP_BUF_LEN             512

#define STR_SDP_ATTRIBUTE_LEN       128

#define SDP_END_TAG                 "\r\n"

#define SDP_TRANSPORT_RTP           "RTP"

#define SDP_TRANSPORT_PROFILE_AVP   "AVP"

#define SDP_SIGN_SEMICOLON          ";"

#define SDP_SIGN_SLASH              "/"

#define SDP_SIGN_COLON              ":"

#define SDP_SIGN_H_LINE             "-"

#define SDP_SIGN_MINUS              SDP_SIGN_H_LINE

#define INVALID_PAYLOAD_TYPE   ((uint8_t)-1)

#define STR_SDP_RTPMAP          std::string("rtpmap:")
#define STR_SDP_FMTP            std::string("fmtp:")
#define STR_SDP_CONTROL_ATTR    std::string("control:")
#define STR_SDP_CONTROL_BASE    std::string("*")
#define STR_SDP_URL             std::string("rtsp://")

#define STR_SDP_RECVONLY_ATTR   std::string("recvonly")
#define STR_SDP_SENDONLY_ATTR   std::string("sendonly")
#define STR_SDP_SENDRECV_ATTR   std::string("sendrecv")

enum TRANS_DIRECTION
{
    TRANS_DIRECTION_RECVONLY     = 0x1,
    TRANS_DIRECTION_SENDONLY     = 0x2,
    TRANS_DIRECTION_SENDRECV     = 0x3,
    TRANS_DIRECTION_MAX
};

enum MEDIA_TYPE_VALUE
{
    MEDIA_TYPE_VALUE_VIDEO  = 0,
    MEDIA_TYPE_VALUE_AUDIO  = 1,
    MEDIA_TYPE_VALUE_DATA   = 2,
    MEDIA_TYPE_VALUE_MAX
};
enum PT_TYPE_VALUE
{
    PT_TYPE_PCMU = 0,
    PT_TYPE_PCMA = 8,
    PT_TYPE_MJPEG = 26,
    PT_TYPE_PRIVATE = 96,
    PT_TYPE_H264 = 98,
    PT_TYPE_H265 = 99,
    PT_TYPE_MAX = 0xFF,
};

#define PCMU_AUDIO             std::string("PCMU")
#define PCMA_AUDIO             std::string("PCMA")
#define AAC_AUDIO              std::string("MPEG4-GENERIC")
#define H264_VIDEO             std::string("H264")
#define H265_VIDEO             std::string("H265")
#define MJPEG_VIDEO            std::string("JPEG")
#define MPEG4_VIDEO            std::string("MP4V-ES")


#define PS_VIDEO_RTPMAP         std::string("PS/90000")
#define H264_VIDEO_RTPMAP       std::string("H264/90000")
#define H265_VIDEO_RTPMAP       std::string("H265/90000")
#define MJPEG_VIDEO_RTPMAP      std::string("JPEG/90000")
#define MPEG4_VIDEO_RTPMAP      std::string("MP4V-ES/90000")
#define DEFAULT_AUDIO_PCMU      std::string("PCMU/8000")
#define DEFAULT_AUDIO_PCMA      std::string("PCMA/8000")

#define JUMP_SPACE(psz) {while (isspace(*(psz)) && (  '\0' != *(psz)))(psz)++;}

typedef std::list<SDP_MEDIA_INFO*>     MEDIA_INFO_LIST;
typedef std::list<SDP_MEDIA_INFO*>::iterator     MEDIA_INFO_IT;

class mk_media_sdp
{
public:
    mk_media_sdp();

    virtual ~mk_media_sdp();

    void setFormatContex(MK_Format_Contex* formatCtx);

    void setUrl(const std::string &strUrl);

    std::string getUrl() const;

    void setRange(const std::string &strRange);

    std::string getRange() const;

    void setSessionName(const std::string &strSessionName);

    std::string getSessionName() const;

    void setConnAddr(const std::string &strConnAddr);

    std::string getConnAddr() const;

    void getVideoInfo(MEDIA_INFO_LIST &infoList);

    void addVideoInfo(const SDP_MEDIA_INFO &info);

    void getAudioInfo(MEDIA_INFO_LIST &infoList);

    void addAudioInfo(const SDP_MEDIA_INFO &info);

    void setTransDirect(TRANS_DIRECTION enTransDirect);

    TRANS_DIRECTION getTransDirect();

    void setSsrc(std::string& strSsrc);

    void setOwner(std::string& strOwner);

	int32_t decodeSdp(const std::string   &strSdp, const std::string   &strContBase);

    int32_t encodeSdp(std::string   &strSdp ,int32_t isplayback,std::string timeRange,time_t startTime,time_t endTime);

    void copy(mk_media_sdp& rtspSdp);

    void makeRtpmap(std::string& strRtpmap,uint8_t ucPT,uint32_t ulClockFre);

    uint8_t getPayloadTypeByRtpmap(std::string& strRtpmap);
	
	int32_t encode200Sdp(std::string &strSdp, int32_t isplayback, std::string timeRange, long startTime, long endTime, std::string AudioOutputIp, uint16_t AudioOutputport);
protected:
    enum _enSDP_STATUS
    {
        SDP_GLOBAL_INFO,
        SDP_VIDEO_INFO,
        SDP_AUDIO_INFO
    };
    bool getNextLine(const std::string &strSdpMsg,uint32_t &nNextStart,std::string  &strBuff) const;

    int32_t parseConnDesc(char *pszBuff);

    int32_t parseTimeDesc(char *pszBuff);

	int32_t parseMediaDesc(char *pszBuff, SDP_MEDIA_INFO*& pMediaInfo);

	int32_t parseAttributes(char *pszBuff, SDP_MEDIA_INFO* pMediaInfo, const std::string  &strContBase);

	int32_t parseGlobalAttributes(char *pszBuff, const std::string  &strContBase);

	int32_t parseMediaAttributes(char *pszBuff, SDP_MEDIA_INFO* pMediaInfo, const std::string  &strContBase);

    void    encodeMediaDesc(std::string &strSdp, uint32_t unStatus); 

private:
    int32_t  createStream(MK_Stream** stream, MK_CodecParameters** codecpar);

protected:
    uint32_t            m_unSession;
    std::string         m_strOwner;
    std::string         m_strSessionName;
    std::string         m_strUrl;
    std::string         m_strConnAddr;
    std::string         m_range;
    std::string         m_strSsrc;
    TRANS_DIRECTION     m_enTransDirect;
    MEDIA_INFO_LIST     m_videoInfoList;
    MEDIA_INFO_LIST     m_audioInfoList;

    uint32_t            m_enParseStatus;
    MK_Format_Contex*   m_pFormatCtx;
};


#endif /* __MK_RTSP_SDP_INCLUDE___ */
