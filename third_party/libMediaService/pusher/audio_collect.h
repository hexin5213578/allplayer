#pragma once

#ifdef _WIN32  

#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#include "mmsystem.h"
#pragma comment(lib,"winmm.lib")
#include <stdint.h>
#include <mutex>

#define  WAVEIN_FRAME_SAMPLE_SUM   640		//每帧640采样点
#define  WAVEIN_BUFFER_QUEUE_SIZE  4		//录音缓存队列大小
#define  MAX_AUDIO_LEN             10*1024	//10K大小

//通道数
typedef enum
{
	WAVEIN_CHANNEL_MONO = 1,    //1通道
	WAVEIN_CHANNEL_STEREO = 2,  //2通道
}WAVEIN_CHANNEL;

//音量值范围定义
typedef enum
{
	AUDIO_VOLUME_MIN = 0,    //音量最小值
	AUDIO_VOLUME_MAX = 100   //音量最大值
}AUDIO_VOLUME;

typedef enum
{
	AUDIO_STATE_INIT_NO = 0x00, //尚未初始化
	AUDIO_STATE_INIT_ALREADY = 0x01, //成功初始化
	AUDIO_STATE_CANNOT_USE = 0x02  //无法使用
}AUDIO_STATE;

typedef enum
{
	VALUE_OK = 0,							//成功
	VALUE_COMMON_ERROR = 1,					//失败
	VALUE_PARAM_ERROR = 2,					//参数错误
	VALUE_DEVICE_NOT_EXIST = 3,				//没有麦克风
	VALUE_MEMORY_ALLOC_ERROR = 4,			//内存申请失败
	VALUE_DEVICE_ALREADY_OPEN_ERROR = 5,	//设备已经打开
	VALUE_DEVICE_OPEN_ERROR = 6,			//设备打开失败
	VALUE_MEMORY_PTR_ERROR = 7,				//指针为空
	VALUE_DEVICE_PREPARE_HEAD_ERROR = 8,	//麦克风准备头失败
	VALUE_DEVICE_ADD_BUFFER_ERROR = 9,		//麦克风添加缓存失败
	VALUE_DEVICE_START_RCOLLECT_ERROR = 10,	//开始录音失败
	VALUE_DEVICE_ALREADY_CLOSE_ERROR = 11,	//设备已经关闭
	VALUE_DEVICE_RESET_ERROR = 12,			//设备复位失败
	VALUE_DEVICE_STOP_ERROR = 13,			//设备停止失败
	VALUE_DEVICE_DELETE_BUFFER_ERROR = 14,	//清除缓存失败
	VALUE_DEVICE_CLOSE_ERROR = 15,			//设备关闭失败

	VALUE_NO_MIXER_ERROR = 16,				//没有混音器
	VALUE_MIXER_ALREDY_OPEN_ERROR = 17,		//混音器已经打开
	VALUE_MIXER_OPEN_ERROR = 18,			//混音器打开失败
	VALUE_MIXER_GET_CAPS_ERROR = 19,		//获取混音器能力失败
	VALUE_MIXER_GET_LINE_CONTROL_ERROR = 20,//获取线路控制失败
	VALUE_MIXER_ALREADY_CLOSE_ERROR = 21,	//混音器已经关闭
	VALUE_MIXER_CLOSE_ERROR = 22,			//混音器关闭失败
	VALUE_MIXER_GET_LINE_DETAIL_ERROR = 23,	//获取线路详情失败
	VALUE_MIXER_SET_LINE_DETAIL_ERROR = 24,	//设置线路详情失败
	VALUE_MIXER_LINE_NO_FOUND_ERROR = 25,	//找不到线路

	VALUE_CALLBACK_REGISTER_ERROR = 26,		//音频数据回调注册失败
	VALUE_NO_INIT_ERROR = 27				//未初始化
}RETURN_VALUE;

//采样频率
typedef enum
{
	WAVEIN_SAMPLE_PER_SEC_8 = 8000,     //8.0 kHz
	WAVEIN_SAMPLE_PER_SEC_11 = 11025,    //11.025 kHz
	WAVEIN_SAMPLE_PER_SEC_16 = 16000,    //16.0 kHz
	WAVEIN_SAMPLE_PER_SEC_22 = 22050,    //22.05 kHz
	WAVEIN_SAMPLE_PER_SEC_44 = 44100,    //44.1 kHz
}WAVEIN_SAMPLE_PER_SEC;

//采样点位数
typedef enum
{
	WAVEIN_BITS_PER_SAMPLE_8 = 8,
	WAVEIN_BITS_PER_SAMPLE_16 = 16
}WAVEIN_BITS_PER_SAMPLE;

//回调函数注册信息
typedef struct
{
	void* pCallBackFun;   //回调函数指针
	void* pUser;      //用户数据
}CALLBACKFUNINFO;

//音频数据格式
typedef struct
{
	unsigned char   ucFormat;           //音频压缩格式
	unsigned long   ulSampleRate;       //采样率
	unsigned char   ucBitsPerSample;    //样点比特数
	unsigned char   ucChannel;          //通道数
}AUDIO_FORMAT_INFO;

typedef struct _SVS_MEDIA_FRAME_HEADER
{
	//uint8_t           nVideo;//1：视频  0：音频：

	uint16_t          nWidth;
	uint16_t          nHeight;
	//uint16_t			 nFps;
	uint16_t          nVideoEncodeFormat;
	uint16_t          nMotionBlocks;

	uint8_t           nID[4];
	uint32_t          nVideoSize;
	uint32_t          nTimeTick;
	struct timeval	  nCurTime;
	uint16_t          nAudioSize;
	uint8_t           bKeyFrame;
	uint8_t           bVolHead;
}SVS_MEDIA_FRAME_HEADER, * PSVS_MEDIA_FRAME_HEADER;

//回调函数的统一接口的通知信息结构体
typedef struct
{
	unsigned long   ulNotifyType;       //通知类型，取值见CUMW_NOTIFY_TYPE
	void* pNotifyInfo;        //通知信息，其意义决定于ulNotifyType，取值见CUMW_NOTIFY_TYPE
	unsigned long   ulNotifyInfoLen;    //pNotifyInfo内容的实际长度，必须正确
}NOTIFY_INFO;

//回调函数的统一接口
//参数pParaInfo为统一通知信息
//参数pUserData为用户数据
typedef long(__stdcall* PCALLBACK_UNIFY)(void* pUserData, unsigned long ulLen, void* pUser);

//录音设备类
class CWaveIn
{
#ifdef CPPUNITTEST
	friend class CWaveInMock;
	friend class CWaveInNewMock;
	friend class CWaveInTest;

#endif

public:
	CWaveIn();
	~CWaveIn();

	// 初始化录音设备
	virtual long Init(unsigned long ulQueueSize);

	// 释放录音设备
	virtual long Release();

	// 打开录音设备
	virtual long Open();

	// 关闭录音设备
	virtual long Close();

	// 录音设备是否打开
	virtual BOOL IsOpen() const;

	// 重复使用缓存
	virtual long ReuseBuffer(WAVEHDR* pWHDR);

	// 注册处理采集音频的回调函数
	virtual long RegisterCallBack(const CALLBACKFUNINFO* pCallBackInfo);

	// 获取音频格式
	virtual long GetAudioFormat(AUDIO_FORMAT_INFO& stInfo) const;

public:
	//音频数据回调函数信息
	CALLBACKFUNINFO m_stUnifyCallBackInfo;

	//统一回调函数指针
	PCALLBACK_UNIFY m_pUnifyCallBackFun;

	//标志是否已关闭
	BOOL m_bClose;

private:
	//录音设备句柄
	HWAVEIN m_hWaveIn;
	//缓存头指针
	WAVEHDR* m_pWhdrIn;
	//数据缓存池
	char* m_pBuff;
	//缓存队列大小
	unsigned long   m_ulQueueSize;
	//帧大小，单位为字节数
	unsigned long   m_ulFrameSize;

	//音频格式信息
	WAVEFORMATEX m_stFormat;
};

class CMixerVolume
{
#ifdef CPPUNITTEST
	friend class CMixerVolumeMock;
	friend class CMixerVolumeNewMock;
	friend class CMixerVolumeTest;
#endif

public:
	CMixerVolume();

	~CMixerVolume();

	//BEGIN V100R001C01B036 MODIFY AQ1D01148 2009-02-13 ligengqiang l00124479 for 支持多混音器
	// 初始化混音器
	virtual long Init();
	//END V100R001C01B036 MODIFY AQ1D01148 2009-02-13 ligengqiang l00124479 for 支持多混音器

	// 释放混音器
	virtual long Release() const;

	// 打开混音器
	virtual long Open();

	// 关闭混音器
	virtual long Close();

	// 混音器是否打开
	virtual BOOL IsOpen() const;

	// 获取值大小
	virtual long GetValue(long& lValue);

	// 设置值大小
	virtual long SetValue(const long lValue);

	// 注册处理麦克风音量改变的回调函数
	virtual long RegisterCallBack(const CALLBACKFUNINFO* pCallBackInfo) const;

protected:
	// 获取目标线路索引
	virtual long GetDstLineIndex(MIXERLINE& stMixerLine, const DWORD dwLineType,
		const UINT cDests, unsigned long& ulIndex);

	// 获取源线路索引
	virtual long GetSrcLineIndex(MIXERLINE& stMixerLine, const DWORD dwLineType,
		const UINT cConns, unsigned long& cIndex);

	// 打开指定索引的混音器
	virtual long Open(unsigned long ulMixerIndex);
	//END V100R001C01B036 ADD AQ1D01148 2009-02-13 ligengqiang l00124479 for 支持多混音器

	//设置混音控制器结构体信息
	virtual void SetMixerControlDetails(MIXERCONTROLDETAILS_SIGNED& stDetail,
		MIXERCONTROLDETAILS& stMixerCtrlDetail);
private:
	// 混音器句柄
	HMIXER m_HMixer;

	// 混音控制器
	MIXERCONTROL m_stMixerCtrl;

	//混音器个数
	unsigned long m_ulMixerNumDevs;
};

class AudioCollect
{
#ifdef CPPUNITTEST
	friend class CReverseAudioIngressAutoMock;
	friend class CReverseAudioIngressTest;
#endif

public:
	static AudioCollect& GetInstance();

	~AudioCollect();

	//初始化反向音频输入
	virtual long Init();

	//释放反向音频输入
	virtual long Release();

	//开始反向音频输入
	virtual long startIngress();

	//停止反向音频输入
	virtual long stopIngress();

	// 获取语音输入实例状态：0，尚未初始化；1，成功初始化；2，无法使用；
	virtual long GetState();

	// 设置麦克风音量
	virtual long SetVolume(long lVolume);

	// 获取对讲音量大小
	virtual long GetVolume(long& lVolume);

	// 获取音频格式
	virtual long GetAudioFormat(AUDIO_FORMAT_INFO& stInfo);

	long GetAudioData(unsigned char* pData, unsigned int* ulLen);
protected:
	//检查是否已经初始化
	virtual long CheckIsInited();

	AudioCollect();
private:
	//统一的回调调用接口
	static long __stdcall HandleUnifyCallBack(void* pUserData, unsigned long ulLen, void* pUser);

	void ResetData();
private:
	// 录音设备状态，见CUMW_REVERSEAUDIO_STATE
	long				m_lWaveInState;
	// 混音设备状态，见CUMW_REVERSEAUDIO_STATE
	long				m_lMixerState;
	// 当前麦克风音量
	long				m_lVolume;
	// 录音设备类
	CWaveIn*			m_pobjWaveIn;
	// 音量控制器
	CMixerVolume*		m_pobjMixerVolume;

	std::mutex			m_mutex;
	//	CLockGuard*	   m_pLockGuard;

	char*				m_pAudioData;	//音频数据内存
	unsigned long		m_ulAudioLen;	//音频数据长度
};

#endif
