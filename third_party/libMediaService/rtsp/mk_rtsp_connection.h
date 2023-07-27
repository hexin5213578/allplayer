#ifndef __MK_RTSP_CONNECTION_INCLUDE_H__
#define __MK_RTSP_CONNECTION_INCLUDE_H__

#include "as.h"
#include "mk_rtsp_defs.h"
#include "mk_media_sdp.h"
#include "mk_rtsp_udp_handle.h"
#include "mk_rtsp_rtp_frame_organizer.h"
#include "mk_client_connection.h"
#include "mk_rtsp_packet.h"
#include "mk_rtsp_rtcp_packet.h"

//TODO 头文件引入判断
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "track/track.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "track/track.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_IOS)
#include "track.h"
#endif

typedef enum RTSP_SESSION_STATUS
{
    RTSP_SESSION_STATUS_INIT     = 0,
    RTSP_SESSION_STATUS_SETUP    = 1,
    RTSP_SESSION_STATUS_PLAY     = 2,
    RTSP_SESSION_STATUS_PAUSE    = 3,
    RTSP_SESSION_STATUS_TEARDOWN = 4,
    RTSP_SOCK5_PROXY_HANDSHAKE   = 5,
    RTSP_SOCK5_PROXY_AUTH        = 6,
    RTSP_SOCK5_PROXY_REQUEST     = 7, 
}RTSP_STATUS;

enum RTSP_INTERLEAVE_NUM
{
    RTSP_INTERLEAVE_NUM_VIDEO_RTP   = 0,
    RTSP_INTERLEAVE_NUM_VIDEO_RTCP  = 1,
    RTSP_INTERLEAVE_NUM_AUDIO_RTP   = 2,
    RTSP_INTERLEAVE_NUM_AUDIO_RTCP  = 3,
    RTSP_INTERLEAVE_NUM_MAX
};

enum RtspRecvState
{
    kNormal,
    kFoundInterleave,
};


#define RTSP_RETRY_INTERVAL     (200 * 1000)

#define RTP_INTERLEAVE_LENGTH   4

#define RTSP_AUTH_TRY_MAX       3

#define RTSP_DIGEST_LENS_MAX    1024

static std::string STR_NULL = std::string("");


class mk_rtsp_connection: public mk_client_connection, public as_tcp_conn_handle,mk_rtsp_rtp_udp_observer
{
public:
    mk_rtsp_connection();
    virtual ~mk_rtsp_connection();

public:
    virtual int32_t start();
    virtual void    stop();
	virtual void    pause();
	virtual void    vcr_control(double start, double scale, double speed);
    virtual int32_t recv_next();
    virtual void    check_client()  override;
    void            set_rtp_over_tcp();
    void            set_vcr_parameter(VcrControllSt& vcrst);
    virtual void    get_rtp_stat_info(RTP_PACKET_STAT_INFO* statinfo);
    virtual void    get_rtsp_sdp_info(char* info,uint32_t lens,uint32_t& copylen);
    void            set_socks5(const char* socks_addr, uint16_t port, const char* user, const char* pass);

public:
    /* override */
    virtual void handle_recv(void) override;
    virtual void handle_send(void) override;

    virtual int32_t handle_rtp_packet(MK_RTSP_HANDLE_TYPE type,as_msg_block* pData) override;
    virtual int32_t handle_rtcp_packet(MK_RTSP_HANDLE_TYPE type, as_msg_block* pData) override;

public:
    struct RstpRequestStruct 
    {
        RstpRequestStruct(enRtspMethods enMethod, std::string strUri, std::string strTransport, std::string ullSessionId = "",
            float scale = 0.0f, float speed = 0.0, double start = -1.0f, double end = -1.0f);

        enRtspMethods m_method;
        std::string m_url;
        std::string m_transport;
        std::string m_ulSession;
        double m_scale, m_speed;
        double m_start, m_end;
    };

protected:
    int32_t processRecvedMessage(const char* pData, uint32_t unDataSize);
    //rtp,rtcp data error and integrity check,0:imcomplete,need more
    int32_t handleRtpRtcpData(const char* pData, uint32_t unDataSize);

    int32_t socks5Greeting();
    int32_t socks5Auth();
    int32_t socks5ConnectionRequest();

protected:
    int32_t handleAnnounceReq(mk_rtsp_packet& rtspMessage);
    int32_t handleSetParameterReq(mk_rtsp_packet& rtspMessage);
    int32_t handleRtspResp(mk_rtsp_packet& rtspMessage);
    virtual int32_t handleRtspDescribeResp(mk_rtsp_packet& rtspMessage);
    virtual int32_t handleRtspSetUpResp(mk_rtsp_packet& rtspMessage);
    int32_t sendMsg(const char* pszData, uint32_t len);
    int32_t setupConnection();

    int32_t sendRtspReq(enRtspMethods enMethod,std::string& strUri,std::string& strTransport, std::string ullSessionId = "",double scale = 1.0);
    int32_t sendRtspReq(RstpRequestStruct& rtspReq);
    
    int32_t sendRtspOptionsReq();
    int32_t sendRtspDescribeReq();
    virtual int32_t sendRtspSetupReq(SDP_MEDIA_INFO* info);
    virtual int32_t sendRtspRecordReq();
	int32_t sendRtspPlayReq(double start = -1.0, double scale = 0.0, double speed = 0.0);
    int32_t sendRtspGetParameterReq();
    int32_t sendRtspAnnounceReq();
    int32_t sendRtspPauseReq();
    int32_t sendRtspTeardownReq();

private:
	int32_t getCurConnectPort(sockaddr *addr, socklen_t addrlen, int32_t family = AF_INET);
    int32_t sendRtcpMessage();
    void    resetRtspConnect();
    void    trimString(std::string& srcString) const;
    int32_t initTracks();
    int32_t getAddrInfo(struct addrinfo** ailist);

protected:
    MEDIA_INFO_LIST              m_mediaInfoList;
    RTSP_STATUS                  m_Status;
    bool                         m_bSetupSuccess;
    std::string                  m_sessionId;
    bool                         m_bSocks;
    bool                         m_bVoiceTalk;
    time_t                       m_ulLastRecv;
    time_t                       m_ulLastRtcpSend;
    time_t                       m_ulRtcpStartTime;
	as_url_t                     m_url;


private:
    as_url_t                     m_sockUrl;
    bool                         m_bAuth;
    as_network_svr*              m_pNetWorker;
    bool                         m_bSetupTcp;
    mk_rtsp_udp_handle*          m_udpHandles[MK_RTSP_UDP_TYPE_MAX];
    mk_media_sdp                 m_sdpInfo;
    mk_rtcp_packet               m_rtcpPacket;
    char                         m_RecvTcpBuf[MAX_BYTES_PER_RECEIVE];
    uint32_t                     m_ulRecvSize;
    uint32_t                     m_ulSeq;
    uint32_t                     m_ulStatusCode;
    volatile AS_BOOLEAN          m_bDoNextRecv;

    typedef std::map<uint32_t,uint32_t>        SEQ_METHOD_MAP;
    typedef SEQ_METHOD_MAP::iterator           SEQ_METHOD_ITER;
    SEQ_METHOD_MAP               m_SeqMethodMap;
  
    as_digest_t                  m_Authen;
    uint32_t                     m_ulAuthenTime;
    std::string                  m_strAuthenticate;
    std::string                  m_strSdpInfo;
	std::string                  m_strConBase;
    MediaDataInfo              m_mediaInfo;
    uint16_t                     m_fragmentIndex;
    uint16_t                     m_fps;
    uint16_t                     m_unDropFps;
    uint32_t                     m_ulRecvByteSize;
    uint64_t                     m_ulStreamSizeKB;
    VcrControllSt                m_vcrSt;

    enum RtspRecvState           m_recvState;
};

class mk_rtsp_server : public as_tcp_server_handle
{
public:
    mk_rtsp_server();
    virtual ~mk_rtsp_server();
    void set_callback(rtsp_server_request cb,void* ctx);
public:
    /* override */
    virtual int32_t handle_accept(const as_network_addr *pRemoteAddr, as_tcp_conn_handle *&pTcpConnHandle);
private:
    rtsp_server_request m_cb;
    void*               m_ctx;
};
#endif /* __MK_RTSP_CONNECTION_INCLUDE_H__ */
