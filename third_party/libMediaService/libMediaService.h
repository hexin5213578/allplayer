#ifndef __LIB_MEDIA_RTSP_H__
#define __LIB_MEDIA_RTSP_H__
#ifdef WIN32
#ifdef LIBMEDIASERVICE_EXPORTS
#define MR_API __declspec(dllexport)
#else
#define MR_API __declspec(dllimport)
#endif
#else
#define MR_API
#endif
//#include <as.h>
#include <stdint.h>
#include <stdarg.h>
#include <vector>
#include <string>
#include "extend/as_frame.h"

class Track;

typedef  void* MR_SERVER;
typedef  void* MR_CLIENT;
typedef  void* MR_WRITER;

enum MR_CLIENT_STATUS {
    MR_CLIENT_STATUS_CONNECTED      = 0,    //建连成功
    MR_CLIENT_CONNECT_FAILED        = 1,    //建连失败
    MR_CLIENT_MEDIA_ATTRIBUTE       = 2,    //SDP描述的媒体信息
    MR_CLIENT_STATUS_HANDSHAKE      = 3,
    MR_CLIENT_STATUS_RUNNING        = 4,
    MR_CLIENT_STATUS_TEARDOWN       = 5,
    MR_CLIENT_STATUS_TIMEOUT        = 6,
    MR_CLIENT_SETUP_TIMEOUT         = 7,
    MR_CLIENT_STATUS_RECV_FAIL      = 8,    //recv()失败
    MR_CLIENT_STATUS_CONN_CLOSE     = 9,    //连接关闭
    MR_CLIENT_STATUS_SRV_ERROR      = 10,    //resp错误
    MR_CLIENT_STATUS_EOS            = 11,   //Announce:HUA_WEI
    MR_CLIENT_STATUS_PAUSE          = 12,   //pause成功
	MR_CLIENT_FRAG_MISTACH          = 13,   //解析出的片段数大于给定值
    MR_CLIENT_VOICE_START_FAIL      = 14,   //语音(对讲，广播etc.)开启失败
    MR_CLIENT_VOICE_START_SUCCESS   = 15,   //语音开启成功
    MR_CLIENT_VOICE_FAIL            = 16,   //语音(对讲，广播etc.)失败
    MR_CLIENT_VOICE_EOS             = 17,   //语音结束
    MK_CLENT_PT_ADAPT               = 18,   //pt随着rtp动态适配
    MR_CLIENT_STATUS_MAX
};

typedef struct _stMEDIA_STATUS_INFO {
    MR_CLIENT_STATUS    enStatus;
    uint32_t            errCode;
}MEDIA_STATUS_INFO;

enum MR_MEDIA_CODE {
    MR_MEDIA_CODE_OK            = 0,
    MR_MEDIA_CODE_MEMORY_OOM    = 0x1001,
    MR_MEDIA_CODE_MAX
};

typedef struct _stRTP_PACKET_STAT_INFO {
    double   dlTraffic;
    double   dlDataTransDuration;
    uint64_t ulStreamSizeKb;
    uint32_t ulTotalPackNum;
    uint32_t ulLostRtpPacketNum;
    uint32_t ulLostFrameNum;
    uint32_t ulDisorderSeqCounts;
}RTP_PACKET_STAT_INFO;

typedef struct _stRational {
    int num; ///< Numerator
    int den; ///< Denominator
} MK_Rational;

typedef struct _stCodecParameters {
    enum MK_MediaType   codec_type;
    enum MKCodecID  codec_id;
    int             format;
    int             profile;
    int             level;

    //Audio only.
    uint64_t        channel_layout;
    int             channels;
    int             sample_rate;

    //Video only.
    int             width;  
    int             height;
    float           video_fps;

    uint8_t*        extradata;
    int             extradata_size;
} MK_CodecParameters;

typedef struct _stSDP_MEDIA_INFO
{
    uint8_t         ucMediaType;
    uint8_t         ucPayloadType;
    uint16_t        usPort;
    std::string     strRtpmap;
    std::string     strFmtp;
    std::string     strControl;
}SDP_MEDIA_INFO;

typedef struct _stStream_INFO {
    int index;
    uint8_t payload;
    int     interval;           //ms 40,default
    MK_Rational time_base;
    MK_CodecParameters* codecpar;
    int interleaved_min, interleaved_max;
    Track*   track;
    SDP_MEDIA_INFO* sdp;
    //void* internal;
    void release();
} MK_Stream;

typedef struct _stFormat_INFO {
    std::vector<MK_Stream*> streams;
    int64_t duration;
    int video_stream;
    int audio_stream;
   
    void release();

    _stFormat_INFO() {
        video_stream = audio_stream = -1;
    }

    virtual ~_stFormat_INFO() {}
} MK_Format_Contex;

struct MediaDataInfo {
    int                     stream_index;
    MKCodecID               codec_id;
    MR_MEDIA_CODE           code;
    int64_t                 pts;
    double                  ntp;
    uint32_t                is_key;
    int32_t                 height;
    int32_t                 width;
    int32_t                 frame_rate;
    //MK_MediaType            pkt_type;
    uint32_t                offset;
    uint16_t                fragment;
};

struct VcrControllSt
{
	double		start = -1.0;
	double      scale = 1.0;
	uint8_t		scaleOrSpeed = 0;
};

enum SpeedEnum
{
	kScale = 0,
	kSpeed = 1,
};
typedef void (*mk_log)(const char* szFileName, int32_t lLine,int32_t lLevel, const char* format,va_list argp);

typedef int32_t (*rtsp_server_request)(MR_SERVER server,MR_CLIENT client);

typedef int32_t (*handle_client_status)(MR_CLIENT client, MEDIA_STATUS_INFO status,void* ctx);

typedef char* (*handle_client_get_buffer)(MR_CLIENT client,uint32_t len,uint32_t& ulBufLen,void* ctx);

typedef int32_t (*handle_client_media)(MR_CLIENT client,MediaDataInfo* dataInfo,uint32_t len,void* ctx);

typedef struct
{
    handle_client_status        m_cb_status;
    handle_client_get_buffer    m_cb_buffer;
    handle_client_media         m_cb_data;
    void*                       ctx;
}MEDIA_CALL_BACK;

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif
    /* init the media  library */
    MR_API int32_t   mk_lib_init(uint32_t EvnCount,mk_log log,uint32_t MaxClient,uint32_t RtpBufCountPerClient);
    
    /* release the media  library */
    MR_API void      mk_lib_release();
    
    /* create a media server handle */
    MR_API MR_SERVER mk_create_rtsp_server_handle(uint16_t port,rtsp_server_request cb,void* ctx);
    
    /* destory the media  server handle */
    MR_API void      mk_destory_rtsp_server_handle(MR_SERVER server);
   
    /* create a media client handle */
    MR_API MR_CLIENT mk_create_client_handle(char* url,MEDIA_CALL_BACK* cb,void* ctx, bool voiceTalk = false);
   
    /* destory the media client handle */
    MR_API void      mk_destory_client_handle(MR_CLIENT client);
    
    /* set socks5 info */
    MR_API void      mk_client_set_socks(MR_CLIENT client, const char* socks_addr, uint16_t port, const char* user, const char* pass);
    
    /* start the media client handle */
    MR_API int32_t   mk_start_client_handle(MR_CLIENT client);
    
    /* stop the media client handle */
    MR_API void      mk_stop_client_handle(MR_CLIENT client);
    
    /* set a media client callback */
    MR_API void      mk_set_client_callback(MR_CLIENT client,MEDIA_CALL_BACK* cb,void* ctx);
    
    /* recv media data from media client */
    MR_API int32_t   mk_recv_next_media_data(MR_CLIENT client);
    
    /* set a media rtsp client media transport tcp*/
    MR_API void      mk_set_rtsp_client_over_tcp(MR_CLIENT client);

    MR_API void      mk_set_vcr_parameter(MR_CLIENT client, VcrControllSt& vcst);
    
    /* set a media rtsp client/server rtp/rtcp udp port */
    MR_API void      mk_set_rtsp_udp_ports(uint16_t udpstart,uint32_t count);
    
    /* get a media rtsp client/server rtp packet stat info */
    MR_API void      mk_get_client_rtp_stat_info(MR_CLIENT client,RTP_PACKET_STAT_INFO* statinfo);
    
    /* get a media rtsp client sdp info */
    MR_API void      mk_get_client_rtsp_sdp_info(MR_CLIENT client,char* sdpInfo,uint32_t lens,uint32_t& copylen);
   
    /* whether send rtcp packet when recv stream */
    MR_API void      mk_set_client_send_rtcp(MR_CLIENT client,bool bsend);

    /*set wav config for broadcast*/
    MR_API int32_t   mk_set_wav_conf(MR_CLIENT client, const char* wav, char loop);

    /*set fragment to adapt split_stream*/
    MR_API void      mk_set_client_fragment_count(MR_CLIENT client, int16_t fragments);

    /* get av format of rtsp stream*/
    MR_API MK_Format_Contex* mk_get_client_av_format(MR_CLIENT client);

	/*pause set*/
	MR_API void      mk_create_rtsp_client_pause(MR_CLIENT client);

	/*speed x2 x4 x8*/
	MR_API void      mk_create_rtsp_play_control(MR_CLIENT client,double start, double scale, double speed);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /*__LIB_MEDIA_RTSP_H__*/