#ifndef __AVS_PLAYER_COMMON_H__
#define __AVS_PLAYER_COMMON_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "as.h"
#include "libMediaService.h"

#define __STDC_CONSTANT_MACROS

struct MediaInfoMsg {
	using Ptr = std::shared_ptr<MediaInfoMsg>;
	MediaDataInfo                 dataInfo;
	uint32_t                      dataSize;
};

#define URL_LEN_MAX					1024
#define RECORD_PATH_MAX             1024
#define SNAP_PATH_MAX               1024

#define H264_FRAME_SIZE_MAX			(1024*1024*4)
#define STREAM_RECV_THREAD_DEFAULT   1
#define PLAYER_MAX_DEFAULT           64

#define RTP_PACKET_BUF_PER_CLIENT    256

#define PLAYER_LOG_MAX               1024
#define MS_COUNT_PER_SEC			 1000

#define INVALID_WND -1

enum BUSSINES_TYPE
{
	TYPE_NONE = 0,									//业务不存在

	TYPE_SOCKS_INIT		=   101,					//开启Socks5代理
	TYPE_SOCKS_CANCEL	=	102,					//取消Socks5代理

	TYPE_REALVIDEO_START = 1001,					//实时浏览开始
	TYPE_REALVIDEO_STOP = 1002,						//实时浏览停止
	TYPE_REALVIDEO_PAUSE = 1003,					//实时浏览暂停
	TYPE_REALVIDEO_RESUME = 1004,					//实时浏览继续

	TYPE_NETRECORD_START = 2001,					//录像回放开始
	TYPE_NETRECORD_STOP = 2002,						//录像回放停止
	TYPE_NETRECORD_PAUSE = 2003,					//录像回放暂停
	TYPE_NETRECORD_RESUME = 2004,					//录像回放继续
	TYPE_NETRECORD_CONTROL = 2005,					//录像回放VCR控制
	TYPE_NETRECORD_SCALE_SET = 2006,				//录像回放倍速设置  (多sdk适配)
	TYPE_NETRECORD_POSITION_SKIP = 2007,			//录像回放位置跳转  (多sdk适配)  
	TYPE_NETRECORD_PLAY_DIRECTION_SET = 2008,		//录像回放播放方向设置  (多sdk适配)   

	TYPE_DOWNLOAD_START = 3001,			//录像下载开始
	TYPE_DOWNLOAD_STOP = 3002,			//录像下载停止
	TYPE_DOWNLOAD_PAUSE = 3003,			//录像下载暂停
	TYPE_DOWNLOAD_CONTINUE = 3004,		//录像下载继续

	TYPE_DIGITAL_START = 4001,			//电子放大开始
	TYPE_DIGITAL_UPDATE = 4002,         //电子放大刷新 
	TYPE_DIGITAL_STOP = 4003,		    //电子放大停止
	TYPE_LOCAL_RECORD_START = 4004,		//本地录像开始
	TYPE_LOCAL_RECORD_STOP = 4005,      //本地录像结束
	TYPE_CAPTURE = 4006,                //截图
	TYPE_VOLUME_CONTROL = 4007,         //音量控制
	TYPE_VOLUME_GET = 4008,             //音量获取
	TYPE_GET_MEDIA_INFO = 4009,         //获取媒体信息
	TYPE_AUDIO_OPEN = 4010,	            //随路语音打开
	TYPE_AUDIO_CLOSE = 4011,	        //随路语音关闭
	TYPE_SKIP_NOKEY_FRAME = 4012,		//只解码关键帧
	TYPE_CAPTURE_FRAMES = 4013,			//按帧抓拍
	TYPE_STEP_FORWARD = 4014,			//单帧播放，前进
	TYPE_STEP_BACKWARD = 4015,			//单帧播放，后退
	TYPE_STEP_EXIT	= 4016,				//退出单帧模式
	TYPE_SET_RtspUrl= 4017,				//设置url地址

	TYPE_AUDIO_TALK_START = 5001,		//语音对讲开始
	TYPE_AUDIO_TALK_STOP = 5002,		//语音对讲关闭
	TYPE_VOICE_BROADCASR_START = 5003,	//语音广播开启		
	TYPE_VOICE_BROADCASR_STOP = 5004,	//语音广播关闭
	TYPE_FILE_BROADCASR_START = 5005,	//文件广播开启	
	TYPE_FILE_BROADCASR_STOP = 5006,	//文件广播结束

	TYPE_GET_EXPERIENCE_DATA = 6001,	//获取体验数据
	TYPE_RENDER_TYPE	=  6002,		//渲染类型
	TYPE_PICTURE_PARAMS	=  6003,		//图像参数
	TYPE_GET_PIC_PARAMS =  6004,		//获取参数值
	TYPE_SET_WATERMARK	=  6005,		//设置水印

	TYPE_URL_START	=		7001,		//Url播放
	TYPE_URL_STOP	=		7002,
	TYPE_URL_PROBE	=		7003,		//探测Url格式
	TYPE_URL_PAUSE	=		7004,
	TYPE_URL_RESUE	=		7005,
	TYPE_URL_SEEK	=		7006,
	TYPE_URL_SPEED	=		7007,		//倍速播放
	TYPE_URL_VOLUME =		7008,		//音量控制
	
	TYPE_MULTI_REALVIDEO_START	=	8001,		//实时拼接流播放
	TYPE_MULTI_REALVIDEO_STOP	=	8002,
	TYPE_MULTI_RECORD_START		=	8003,		//录像拼接流播放
	TYPE_MULTI_RECORD_STOP		=	8004,
	TYPE_MULTI_RECORD_PAUSE		=	8005,        //录像回放暂停
	TYPE_MULTI_RECORD_RESUME	=	8006,        //录像回放继续
	TYPE_MULTI_RECORD_CONTROL	=	8007,        //录像回放VCR控制
};

//业务状态
typedef enum {
	EGRESS_STATUS_NONE		=	-1,
	OPEN_DECODER_ERROR		=	10001,				//解码器初始化失败
	RECORD_START			=	10002,		
	RECORD_INIT_ERROR		=	10003,
	RECORD_STOP				=	10004,
	DOWNLOAD_START			=	10005,
	DOWNLOAD_INIT_ERROR		=	10006,
	DOWNLOAD_STOP			=	10007,
	FILE_FRAGMENT			=	10008,				//文件分段
	URL_THUMBNAIL_SUCCESS	=	10009,				//获取文件缩略图，废弃
	URL_THUMBNAIL_FAILED	=	10010,				//废弃
	CAPTURE_FRAME_SUCEESS	=   10011,				//截取一帧
	CAPTURE_FRAME_FAILED	=	10012,
	STRP_FORWARD_RANGE		=	10013,				//单帧前向边界				
	SETP_BACKWARD_RANGE		=	10014,
	URL_EOF					=	10015,		
	SET_TEXTURE_FAILED		=	10016,				//设置渲染texture失败
	FILE_WRITE_ERROR		=	10017,				//文件写入失败(无磁盘空间，分辨率变化etc.)
	DOWNLOAD_KBYTERATE		=	10018,				//下载速度(单位:kBps)
	STATUS_MAX				
}EGRESS_STATUS_TYPE;

typedef enum {
	AVS_STATUS_NONE			= -1,
	AVS_INVALID				= 11000,					//无效/不支持的AVS格式(文件被破坏导致无法初始化资源)
	AVS_PLAY_FAILED			= 11001,					//播放失败
	AVS_EOF					= 11002,					//播放结束
}AVS_EGRESS_STATUS;

//视频数据流状态类型定义
typedef enum {
	STREAM_STATUS_NONE			=	-1,
	STREAM_STATUS_CONNECTED		=	20001,				//媒体连接已建立
	STREAM_STATUS_KEYFRAME		=	20002,				//视频渲染第一帧
	STREAM_STATUS_PAUSE			=	20003,				//暂停
	STREAM_STATUS_RESUME		=	20004,				//恢复
	STREAM_STATUS_TEARDOWN		=	20005,				//rtsp流通过tearDown关闭
	STREAM_STATUS_TIMEOUT		=	20006,				//数据流接收超时
	STREAM_STATUS_STOP			=	20007,				//资源回收完毕
	STREAM_STATUS_SETUP_TIMEOUT	=	20008,				//describe/setup信令超时
	STREAM_STATUS_CONN_ERROR	=	20009,				//服务器返回错误等异常情况
	STREAM_STATUS_SRV_ERROR		=	20010,				//服务端错误
	STREAM_STATUS_EOS			=	20011,				//end of stream
	STREAM_STATUS_SETUP			=	20012,				//RTSP SETUP成功，可PLAY
	STREAM_STATUS_IO_FINISH		=	20013,				//io结束(写mp4)
	STREAM_CONNECT_FAILED		=	20014,				//connect失败(超时时间：1.5s)
	STREAM_STATUS_PAUSE_RESP	=	20015,				//pause返回200 ok
	STREAM_STATUS_CONN_CLOSE	=	20016,				//连接关闭(recv返回0,由服务端主动关闭连接)
	STREAM_MULTI_FRAGS_MISMATCH =	20017,				//拼接流多片段数量不匹配
	STREAM_VOICE_START_FAIL		=	20018,				//语音开启失败
	STREAM_VOICE_START_SUCCESS	=	20019,				//语音开启成功 
	STREAM_VOICE_FAIL			=	20020,				//语音对讲失败(开启成功后)
	STREAM_VOICE_EOS			=	20021,				//语音停止 
	STREAM_STATUS_MAX,
} STREAM_STATUS_TYPE;

//视频异常数据上报类型
typedef enum
{
	VIDEO_STALLING_EVENT = 50001, //卡顿异常事件
	VIDEO_FREEZING_EVENT = 50002, //跳帧异常事件
	VIDEO_EVENT_MAX,
}VIDEO_UPLOAD_TYPE;


typedef enum
{
	SDL_SOFT = 0,
	SDL_ACCELERATED = 1,
	OPENGL = 2,
}RENDER_TYPE;

typedef enum {
	kSoft = 0,
	kHardware = 1,
}EDecodeType;

typedef enum {
	JPG = 0,
	BMP = 1,
}PicFormat;

struct ExcuteInfo {
	long	bizId;
	int		bizType;
	void*	varInfo;
};

struct ExpCollector {
	long*	bizArr;
	int		count;

	ExpCollector(int count) {
		this->count = count;
		bizArr = AS_NEW(bizArr, count);
		assert(bizArr);
	}

	virtual ~ExpCollector() {
		AS_DELETE(bizArr, (count > 1) ? MULTI : SINGLE);
	}
};

struct ZoomSt {
	void*		zoomHwnd;				
	int32_t		top;					//电子放大区域(百分比)
	int32_t		bottom;
	int32_t		left;
	int32_t		right;

	ZoomSt() :zoomHwnd(nullptr), top(0), bottom(0), left(0), right(0) { 
	}
};

struct CaptureSt {
	std::string	capturePath;
	uint16_t	captureCount = 1;				//抓拍数量
	CaptureSt(const std::string& path, uint16_t count)
		:capturePath(path), captureCount(count) {
	}
};

struct VoiceSt {
	long			bizId;
	int32_t         bizType;
	std::string		voiceUrl;
	std::string		wavFile;						//Wav文件位置
	char			loop;							//0:单次; 1:循环

	VoiceSt(long id, int32_t type): bizId(id), bizType(type) {
		loop = 0;
	}
};


struct SeekSt {
	double	position;						//seek的位置(百分比)
};

struct FileDumpSt {
	enum formatEnum {
		kMP4 = 0,
		kAVS = 1,
		KMax = 2,
	};
	
	enum cutEnum {
		kTime = 0,
		kSize = 1,
	};

	double			scale = 1.0;
	std::string		filePath;
	formatEnum		fileFormat = kMP4;
	uint8_t			cutFormat = kSize;			//mp4文件切割方式 0-按时间/1-按大小	
	uint32_t		cutDuration = 10;			//按时间切割时长,单位：分钟
	uint32_t		cutSize = 50;				//按大小切割规格,单位：M

	std::string		key;						
	uint32_t		limit_days;
	uint32_t		limit_times;
};

struct StitchSt {
	int16_t			streams = 1;			//多片段数量
	vector<void*>	HWNDs;					//多片段对应的窗口句柄
};

typedef struct Businessinfo {
	long						 BizId;
	int32_t                      BizType;
	std::string                  Url;
	std::string					 CameraId;
	EDecodeType					 DecodeType = kSoft;			//解码类型
	void*						 WindowsHandle = (void*)INVALID_WND;
	uint8_t						 PlayMode = 0;					//本地文件模式时:0-mp4格式，1-avs加密格式
	uint32_t					 CacheSize = 1;					//缓冲帧数量	
	uint8_t						 ScaleOrSpeed = 0;				//平台类型,vcr控制字段, 0:scale, 1:speed
	std::string                  SnapPath;		
	PicFormat                    SnapFormat = JPG;				//抓拍文件类型 0-jpg 1-bmp
	int16_t                      VolumeControl = -1;			//-1:随路语音关闭
	FileDumpSt					 FileDump;						//文件导出
	StitchSt					 StitchInfo;					//拼接流信息
	VcrControllSt				 vcrSt;							//视频播放时的vcr参数
	uint8_t						 HighSpeedCache = 0;            //高倍速缓存 0:不开启，1:开启，默认不开启
}BizParams;

#endif /* __AVS_PLAYER_COMMON_H__ */
