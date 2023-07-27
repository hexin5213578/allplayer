﻿#ifndef __MK_CLIENT_CONNECTION_INCLUDE_H__
#define __MK_CLIENT_CONNECTION_INCLUDE_H__

#include "as.h"
#include "libMediaService.h"

enum MK_CLIENT_TYPE
{
    MK_CLIENT_TYPE_RTSP = 0,
    MK_CLIENT_TYPE_RTMP = 1,
    MK_CLIENT_TYPE_MAX
};

#define MK_CLIENT_TIMER_INTERVAL        100
#define MK_CONNECT_TIMEOUT              2000
#define MK_CLIENT_RECV_TIMEOUT          6
#define MK_CLIENT_SEND_RTCP_INTERVAL    3

class mk_client_timer:public ITrigger
{
public:
    mk_client_timer() = default;
    virtual ~mk_client_timer() = default;
    virtual void onTrigger(void *pArg, ULONGLONG ullScales, TriggerStyle enStyle);
};

class mk_client_connection
{
public:
    mk_client_connection();
    virtual ~mk_client_connection();
    MK_CLIENT_TYPE client_type();
    MR_CLIENT_STATUS get_status();
    void     set_url(const char* pszUrl);
    void     set_status_callback(MEDIA_CALL_BACK* cb,void* ctx);
    int32_t  start_recv();
    int32_t  stop_recv();
    int32_t  do_next_recv();
    void     set_index(uint32_t ulIdx);
    uint32_t get_index();
    void     get_client_rtp_stat_info(RTP_PACKET_STAT_INFO* statinfo);
    void     get_client_rtsp_sdp_info(char* info,uint32_t lens,uint32_t& copylen);
    void     set_client_send_rtcp(bool bsend);
    void     set_client_fragment_count(int16_t fragments);
    int16_t  get_client_frags();
    MK_Format_Contex* get_client_av_format();

public:
    virtual int32_t start() = 0;
    virtual void    stop() = 0;
    virtual int32_t recv_next() = 0;
    virtual void    check_client() = 0;
    virtual void    get_rtp_stat_info(RTP_PACKET_STAT_INFO* statinfo) = 0;
    virtual void    get_rtsp_sdp_info(char* info,uint32_t lens,uint32_t& copylen) = 0;

    void    handle_connection_media(MediaDataInfo* dataInfo,uint32_t len);
    void    handle_connection_status(MR_CLIENT_STATUS status, uint32_t prot = 200);
    void    handle_connection_status(MEDIA_STATUS_INFO statusInfo);
    char*   handle_connection_databuf(uint32_t len,uint32_t& bufLen);
 
protected:
    int16_t              m_fragements;
    std::string          m_strurl;
    bool                 m_bSendRtcp;
    MK_Format_Contex*    m_pFormatCtx;
private:
    uint32_t             m_ulIndex;
    MK_CLIENT_TYPE       m_enType;
    MR_CLIENT_STATUS     m_enStatus;

    MEDIA_CALL_BACK*     m_ClientCallBack;
    void*                m_pClientCtx;
    mk_client_timer      m_ClientCheckTimer;
};

#endif /* __MK_CLIENT_CONNECTION_INCLUDE_H__ */
