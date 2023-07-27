#include "libMediaService.h"
#include "mk_media_service.h"
#include "mk_client_connection.h"
#include "mk_voice_connection.h"

mk_log g_log = NULL;

/* init the media rtsp libary */
MR_API int32_t   mk_lib_init(uint32_t EvnCount,mk_log log,uint32_t MaxClient,uint32_t RtpBufCountPerClient)
{
    g_log =  log;
    return mk_media_service::instance().init(EvnCount,MaxClient,RtpBufCountPerClient);
}

/* release the media rtsp bibary */
MR_API void      mk_lib_release()
{
    mk_media_service::instance().release();
    return;
}

/* create a media rtsp server handle */
MR_API MR_SERVER mk_create_rtsp_server_handle(uint16_t port,rtsp_server_request cb,void* ctx)
{
    return mk_media_service::instance().create_rtsp_server(port,cb,ctx);
}

/* destory the media rtsp server handle */
MR_API void      mk_destory_rtsp_server_handle(MR_SERVER server)
{
    mk_rtsp_server* pServer = (mk_rtsp_server*)server;
    mk_media_service::instance().destory_rtsp_server(pServer);
    return;
}

/* create a media rtsp client handle */
MR_API MR_CLIENT mk_create_client_handle(char* url,MEDIA_CALL_BACK* cb,void* ctx, bool voiceTalk)
{
    return mk_media_service::instance().create_client(url,cb,ctx, voiceTalk);
}

/* destory the media client handle */
MR_API void      mk_destory_client_handle(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    if (pClient) {
        mk_media_service::instance().destory_client(pClient);
    }
    return;
}

MR_API void mk_client_set_socks(MR_CLIENT client, const char* socks_addr, uint16_t port, const char* user, const char* pass)
{
    mk_rtsp_connection* pClient = (mk_rtsp_connection*)client;
    if (pClient) {
        pClient->set_socks5(socks_addr, port, user, pass);
    }
    return;
}

/* start the media client handle */
MR_API int32_t   mk_start_client_handle(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    if (pClient) {
        return pClient->start_recv();
    }
    return -1;
}

/* stop the media client handle */
MR_API void   mk_stop_client_handle(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    if (pClient) {
        pClient->stop_recv();
    }
    return;
}

/* set a media client callback */
MR_API void      mk_set_client_callback(MR_CLIENT client,MEDIA_CALL_BACK* cb,void* ctx)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    if (pClient) {
        pClient->set_status_callback(cb, ctx);
    }
    return;
}

/* recv media data from media client */
MR_API int32_t   mk_recv_next_media_data(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    if (pClient) {
        return pClient->do_next_recv();
    }
    return -1;
}

/* set a media rtsp client media transport tcp */
MR_API void      mk_set_rtsp_client_over_tcp(MR_CLIENT client)
{
    mk_rtsp_connection* pClient = (mk_rtsp_connection*)client;
    if (pClient) {
        pClient->set_rtp_over_tcp();
    }
    return;
}

MR_API void  mk_set_vcr_parameter(MR_CLIENT client, VcrControllSt& vcst)
{
	mk_rtsp_connection* pClient = (mk_rtsp_connection*)client;
	if (pClient) {
		pClient->set_vcr_parameter(vcst);
	}
	return;
}

/*set a media rtsp pause*/
MR_API void     mk_create_rtsp_client_pause(MR_CLIENT client)
{
	mk_rtsp_connection* pClient = static_cast<mk_rtsp_connection*>(client);
    if (pClient) {
        pClient->pause();
    }
	return;
}

/*set a media rtsp speed*/
MR_API void     mk_create_rtsp_play_control(MR_CLIENT client, double start, double scale, double speed)
{
	mk_rtsp_connection* pClient = static_cast<mk_rtsp_connection*>(client);
    if (pClient) {
        pClient->vcr_control(start, scale, speed);
    }
	return;
}

MR_API int mk_set_wav_conf(MR_CLIENT client, const char* wav, char loop)
{
    voice_rtsp_connection* voice_client = dynamic_cast<voice_rtsp_connection*>((mk_rtsp_connection*)client);
    if (voice_client) {
        return voice_client->set_wav_config(wav, loop);
    }
    return AS_ERROR_CODE_NOT_INIT;
}

MR_API MK_Format_Contex* mk_get_client_av_format(MR_CLIENT client)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    if (pClient) {
        return pClient->get_client_av_format();
    }
    return nullptr;
}

/* set a media rtsp client/server rtp/rtcp udp port */
MR_API void      mk_set_rtsp_udp_ports(uint16_t udpstart,uint32_t count)
{
    return;
}

/* get a media rtsp client/server rtp packet stat info */
MR_API void      mk_get_client_rtp_stat_info(MR_CLIENT client,RTP_PACKET_STAT_INFO* statinfo)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->get_client_rtp_stat_info(statinfo);
    return;
}

/* get a media rtsp client sdp info */
MR_API void      mk_get_client_rtsp_sdp_info(MR_CLIENT client,char* sdpInfo,uint32_t lens,uint32_t& copylen)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->get_client_rtsp_sdp_info(sdpInfo,lens,copylen);
    return;
}

/* whether send rtcp packet when recv stream */
MR_API void      mk_set_client_send_rtcp(MR_CLIENT client,bool bsend)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    pClient->set_client_send_rtcp(bsend);
    return;
}

MR_API void mk_set_client_fragment_count(MR_CLIENT client, int16_t fragments)
{
    mk_client_connection* pClient = (mk_client_connection*)client;
    if (pClient) {
        pClient->set_client_fragment_count(fragments);
    }
    return;
}

void _stFormat_INFO::release()
{
    for (auto iter = streams.begin(); iter != streams.end(); ++iter) {
        if (*iter != nullptr) {
            (*iter)->release();
            MK_Stream* stream = *iter;
            AS_DELETE(stream);
        }
    }
   
    streams.clear();
    vector<MK_Stream*> swapVector;
    swapVector.swap(streams);
}

void _stStream_INFO::release()
{
    AS_DELETE(track);
    AS_DELETE(codecpar);
}
