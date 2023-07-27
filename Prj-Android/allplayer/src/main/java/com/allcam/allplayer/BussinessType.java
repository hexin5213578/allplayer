package com.allcam.allplayer;

/**
 * @ClassName BussinessType
 * @Description 传递给C++层的动作标识
 * @Author liufang
 * @Date 2022/10/10 20:48
 * @Version 1.0
 */
interface BussinessType {
  int  TYPE_NONE = 0,									//业务不存在

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
    TYPE_SET_URL	= 4017,				//设置url地址

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


    TYPE_URL_START	=		7001,		//Url播放  本地播放开启
    TYPE_URL_STOP	=		7002,
    TYPE_URL_THUMBNAIL =	7003,
    TYPE_URL_PAUSE	=		7004,
    TYPE_URL_RESUE	=		7005,
    TYPE_URL_SEEK	=		7006,

    TYPE_MULTI_REALVIDEO_START	=	8001,		//实时拼接流播放
    TYPE_MULTI_REALVIDEO_STOP	=	8002,
    TYPE_MULTI_RECORD_START		=	8003,		//录像拼接流播放
    TYPE_MULTI_RECORD_STOP		=	8004,
    TYPE_MULTI_RECORD_PAUSE		=	8005,        //录像回放暂停
    TYPE_MULTI_RECORD_RESUME	=	8006,        //录像回放继续
    TYPE_MULTI_RECORD_CONTROL	=	8007,       //录像回放VCR控制


    TYPE_VIDEO_PLAY_INFO = 6001, // 获取体验数据的标识
    TYPE_DEVICE_MEDIA_INFO = 4009;// 获取镜头
}
