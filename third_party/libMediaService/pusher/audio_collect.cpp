#include "audio_collect.h"

#ifdef _WIN32  

#include "stdio.h"
#include "as_mem.h"

void CALLBACK WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	//暂只处理MM_WIM_DATA消息
	if (MM_WIM_DATA != uMsg) {
		return;
	}

	//解析参数
	CWaveIn* pobjWaveIn = (CWaveIn*)dwInstance; //CWaveIn对象实例
	WAVEHDR* pWHDR = (WAVEHDR*)dwParam1;   //音频数据缓存头

	if (pobjWaveIn->m_bClose) {
		return;
	}

	if (NULL == pobjWaveIn->m_pUnifyCallBackFun) {
		return;
	}

	//begin 新增帧头
	unsigned long ulFrameLen = pWHDR->dwBufferLength;
	char* pData = NULL;
	if (NULL == AS_NEW(pData, ulFrameLen)) {
		(void)pobjWaveIn->ReuseBuffer(pWHDR);
		return;
	}
	memset(pData, 0, ulFrameLen);
	memcpy(pData, pWHDR->lpData, pWHDR->dwBufferLength);
	PCALLBACK_UNIFY pFun = (PCALLBACK_UNIFY)(pobjWaveIn->m_pUnifyCallBackFun);
	pFun(pData, ulFrameLen, pobjWaveIn->m_stUnifyCallBackInfo.pUser);

	AS_DELETE(pData, 1);
	//重新将缓存添加到缓存队列
	pobjWaveIn->ReuseBuffer(pWHDR);
}

CWaveIn::CWaveIn()
{
	m_hWaveIn = NULL; //录音设备句柄
	m_pWhdrIn = NULL; //缓存头指针
	m_pBuff = NULL; //数据缓存池指针

	m_ulQueueSize = 0;    //缓存队列大小
	m_ulFrameSize = 0;    //帧大小

	m_pUnifyCallBackFun = NULL;
	//音频数据回调函数信息
	memset(&m_stUnifyCallBackInfo, 0, sizeof(m_stUnifyCallBackInfo));
	//音频格式信息
	memset(&m_stFormat, 0, sizeof(m_stFormat));
	//标志是否已关闭
	m_bClose = TRUE;
}

CWaveIn::~CWaveIn()
{
	m_hWaveIn = NULL; //录音设备句柄
	m_pWhdrIn = NULL; //缓存头指针
	m_pBuff = NULL; //数据缓存池指针
}

/******************************************************************************
Function:    Init
Description: 初始化录音设备
ulQueueSize:    缓存队列大小
Return:      错误码
******************************************************************************/
long CWaveIn::Init(unsigned long ulQueueSize)
{
	//缓存队列大小必须大于0
	if (0 == ulQueueSize) {
		return VALUE_PARAM_ERROR;
	}

	//检测是否有WaveIn设备
	if (0 == waveInGetNumDevs()) {
		return VALUE_DEVICE_NOT_EXIST;
	}

	//保存缓存队列大小
	m_ulQueueSize = ulQueueSize;

	//计算帧的大小，单位为字节数
	//计算方法：
	//  1、采样点数 * 采样点位数
	//  2、再将位数转化为字节数，即除以8
	m_ulFrameSize = WAVEIN_FRAME_SAMPLE_SUM * WAVEIN_BITS_PER_SAMPLE_16 / 8;

	//分配缓存头,并初始化为全0
	if (NULL == AS_NEW(m_pWhdrIn, m_ulQueueSize)) {
		return VALUE_MEMORY_ALLOC_ERROR;
	}
	memset(m_pWhdrIn, 0, sizeof(WAVEHDR) * m_ulQueueSize);

	//缓存空间大小
	unsigned long ulBuffLen = m_ulFrameSize * m_ulQueueSize;

	//分配缓存池空间，并初始化为全0
	if (NULL == AS_NEW(m_pBuff, ulBuffLen)) {
		return VALUE_MEMORY_ALLOC_ERROR;
	}
	memset(m_pBuff, 0, ulBuffLen);

	//当前帧缓存指针
	char* pCurFrameBuff = m_pBuff;
	//遍历缓存队列
	for (unsigned long i = 0; i < m_ulQueueSize; i++) {
		//调用waveInPrepareHeader前必须设置的三个成员
		m_pWhdrIn[i].dwBufferLength = m_ulFrameSize;    //Buffer长度
		m_pWhdrIn[i].dwFlags = 0;                //必须置0
		m_pWhdrIn[i].lpData = pCurFrameBuff;    //Buffer指针
		//下一帧首地址
		pCurFrameBuff += m_ulFrameSize;
	}

	//设置音频格式
	m_stFormat.wFormatTag = WAVE_FORMAT_PCM;                  //编码格式
	m_stFormat.nChannels = WAVEIN_CHANNEL_MONO;				  //通道数
	m_stFormat.nSamplesPerSec = WAVEIN_SAMPLE_PER_SEC_8;      //采样频率
	m_stFormat.wBitsPerSample = WAVEIN_BITS_PER_SAMPLE_16;    //采样位数
	//块对齐
	m_stFormat.nBlockAlign = (WORD)(m_stFormat.nChannels * m_stFormat.wBitsPerSample / 8);
	//每秒的字节数
	m_stFormat.nAvgBytesPerSec = m_stFormat.nSamplesPerSec * m_stFormat.nBlockAlign;
	m_stFormat.cbSize = 0;    //WAVE_FORMAT_PCM时，为0
	return VALUE_OK;
}

// 释放录音设备
long CWaveIn::Release()
{
	AS_DELETE(m_pWhdrIn, MULTI);   //释放缓存头空间
	AS_DELETE(m_pBuff, MULTI);     //释放数据缓存池空间
	return VALUE_OK;
}

/******************************************************************************
Function:    Open
Description: 打开录音设备
Return:      错误码
******************************************************************************/
long CWaveIn::Open()
{
	//检测是否打开录音设备
	if (IsOpen()) {
		return VALUE_DEVICE_ALREADY_OPEN_ERROR;
	}

	//检测指针是否为空
	if (NULL == m_pWhdrIn) {
		return VALUE_MEMORY_PTR_ERROR;
	}

	//待增加
	//检测是否已注册回调函数

	//WaveIn系列函数调用返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//打开录音设备，采用自定义回调处理相关消息
	//WAVE_MAPPER为自动选择录音设备
	DWORD fdwOpen = (DWORD)(CALLBACK_FUNCTION); /*lint !e620 系统#define CALLBACK_FUNCTION   0x00030000l 宏末尾的l的使用不会有问题 */
	MMResult = waveInOpen(&m_hWaveIn, WAVE_MAPPER, &m_stFormat, (DWORD_PTR)WaveInProc, (DWORD_PTR)this, fdwOpen);

	if (MMSYSERR_NOERROR != MMResult) {
		//打开失败，则重置录音设备句柄为NULL
		m_hWaveIn = NULL;
		return VALUE_DEVICE_OPEN_ERROR;
	}

	//遍历计数器
	unsigned long i = 0;

	//遍历缓存队列个数，准备缓存
	for (i = 0; i < m_ulQueueSize; i++) {
		//准备缓存
		MMResult = waveInPrepareHeader(m_hWaveIn, &m_pWhdrIn[i], sizeof(WAVEHDR));
		if (MMSYSERR_NOERROR != MMResult) {
			(void)Close();  //不需处理关闭录音设备返回值
			return VALUE_DEVICE_PREPARE_HEAD_ERROR;
		}
	}

	//遍历缓存队列个数，添加缓存
	for (i = 0; i < m_ulQueueSize; i++) {
		//添加缓存
		MMResult = waveInAddBuffer(m_hWaveIn, &m_pWhdrIn[i], sizeof(WAVEHDR));
		if (MMSYSERR_NOERROR != MMResult) {
			(void)Close();  //不需处理关闭录音设备返回值
			return VALUE_DEVICE_ADD_BUFFER_ERROR;
		}
	}

	//开始录音
	MMResult = waveInStart(m_hWaveIn);
	if (MMSYSERR_NOERROR != MMResult) {
		(void)Close();  //不需处理关闭录音设备返回值
		return VALUE_DEVICE_START_RCOLLECT_ERROR;
	}

	//设置标志为未关闭
	m_bClose = FALSE;
	return VALUE_OK;
}

/******************************************************************************
Function:    Close
Description: 关闭录音设备
Return:      错误码
******************************************************************************/
long CWaveIn::Close()
{
	//检测是否已打开
	if (!IsOpen()) {
		return VALUE_DEVICE_ALREADY_CLOSE_ERROR;
	}

	long lResult = VALUE_OK;    //本函数返回值
	MMRESULT MMResult = MMSYSERR_NOERROR; //WaveIn系列函数调用返回值

	//设置标志为已关闭
	m_bClose = TRUE;

	//停止录音
	MMResult = waveInReset(m_hWaveIn);
	if (MMSYSERR_NOERROR != MMResult) {
		lResult = VALUE_DEVICE_RESET_ERROR;
	}

	MMResult = waveInStop(m_hWaveIn);
	if (MMSYSERR_NOERROR != MMResult) {
		lResult = VALUE_DEVICE_STOP_ERROR;
	}

	//遍历缓存队列个数
	for (unsigned long i = 0; i < m_ulQueueSize; i++) {
		//检测指针是否为空
		if (NULL == m_pWhdrIn) {
			lResult = VALUE_MEMORY_PTR_ERROR;
			//直接退出循环
			break;
		}

		//清除缓存
		MMResult = waveInUnprepareHeader(m_hWaveIn, &m_pWhdrIn[i], sizeof(WAVEHDR));

		if (MMSYSERR_NOERROR != MMResult) {
			lResult = VALUE_DEVICE_DELETE_BUFFER_ERROR;
		}
	}

	//关闭录音设备
	MMResult = waveInClose(m_hWaveIn);
	if (MMSYSERR_NOERROR != MMResult)
	{
		lResult = VALUE_DEVICE_CLOSE_ERROR;
	}

	//重置WaveIn句柄
	m_hWaveIn = NULL;
	return lResult;
}

/******************************************************************************
Function:    IsOpen
Description: 录音设备是否打开
Return:      错误码
******************************************************************************/
BOOL CWaveIn::IsOpen() const
{
	//默认为未打开
	BOOL bResult = FALSE;

	//句柄非空，则认为已打开
	if (NULL != m_hWaveIn) {
		bResult = TRUE;
	}
	return bResult;
}

/******************************************************************************
Function:    ReuseBuffer
Description: 重复使用缓存
pWHDR:  缓存地址
Return:      错误码
******************************************************************************/
long CWaveIn::ReuseBuffer(WAVEHDR* pWHDR)
{
	//检测是否已关闭
	if (m_bClose) {
		return VALUE_OK;
	}

	//参数检查
	if (NULL == pWHDR) {
		return VALUE_PARAM_ERROR;
	}

	//检测是否打开录音设备
	if (!IsOpen()) {
		return VALUE_DEVICE_OPEN_ERROR;
	}
	//WaveIn系列函数调用返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//添加缓存
	MMResult = waveInAddBuffer(m_hWaveIn, pWHDR, sizeof(WAVEHDR));

	if (MMSYSERR_NOERROR != MMResult) {
		return VALUE_DEVICE_ADD_BUFFER_ERROR;
	}
	return VALUE_OK;
}

/******************************************************************************
Function:    RegisterCallBack
Description: 注册处理采集音频的回调函数
pCallBackInfo:  回调函数指针
Return:      错误码
******************************************************************************/
long CWaveIn::RegisterCallBack(const CALLBACKFUNINFO* pCallBackInfo)
{
	//参数检查
	if (NULL == pCallBackInfo) {
		return VALUE_PARAM_ERROR;
	}

	//设置音频数据回调函数信息
	//转换的对象指针为函数地址,将其转换为函数指针不会产生问题
	memcpy(&m_stUnifyCallBackInfo, pCallBackInfo, sizeof(m_stUnifyCallBackInfo));
	m_pUnifyCallBackFun = (PCALLBACK_UNIFY)(m_stUnifyCallBackInfo.pCallBackFun); /*lint !e611 被转换的指针必然为PCALLBACK_UNIFY类型，所以不会出现问题 */
	return VALUE_OK;
}

/******************************************************************************
Function:    GetAudioFormat
Description: 获取音频格式
stInfo:     音频格式信息
Return:      错误码
******************************************************************************/
long CWaveIn::GetAudioFormat(AUDIO_FORMAT_INFO& stInfo) const
{
	stInfo.ucFormat = (unsigned char)m_stFormat.wFormatTag; //编码格式
	stInfo.ucChannel = (unsigned char)m_stFormat.nChannels;  //通道数
	stInfo.ulSampleRate = m_stFormat.nSamplesPerSec;    //采样频率
	stInfo.ucBitsPerSample = (unsigned char)m_stFormat.wBitsPerSample; //采样位数
	return VALUE_OK;
}

CMixerVolume::CMixerVolume()
{
	m_HMixer = NULL;    //混音器句柄
	m_ulMixerNumDevs = 0;   //混音器个数
	memset(&m_stMixerCtrl, 0, sizeof(m_stMixerCtrl));    //混音控制器
}

CMixerVolume::~CMixerVolume()
{
	m_HMixer = NULL;    //混音器句柄
}

/******************************************************************************
Function:    Init
Description: 初始化
Return:      错误码
******************************************************************************/
long CMixerVolume::Init()
{
	//获取混音器个数
	m_ulMixerNumDevs = mixerGetNumDevs();

	//混音器个数为0，无法设置音量
	if (0 == m_ulMixerNumDevs) {
		return VALUE_NO_MIXER_ERROR;
	}
	return VALUE_OK;
}

/******************************************************************************
Function:    Release
Description: 释放混音器
Return:      错误码
******************************************************************************/
long CMixerVolume::Release() const
{
	//暂无资源需要释放
	return VALUE_OK;
}

/******************************************************************************
Function:    Open
Description: 打开混音器
Return:      错误码
******************************************************************************/
long CMixerVolume::Open()
{
	//检测是否打开混音期
	if (IsOpen()) {
		return VALUE_MIXER_ALREDY_OPEN_ERROR;
	}

	//本函数返回值
	long lResult = VALUE_OK;

	for (unsigned long i = 0; i < m_ulMixerNumDevs; i++) {
		lResult = Open(i);

		if (VALUE_OK == lResult) {
			break;
		}
	}
	return lResult;
}

/******************************************************************************
Function:    Open
Description: 打开指定索引的混音器
ulMixerIndex:   混音器索引
Return:      错误码
******************************************************************************/
long CMixerVolume::Open(unsigned long ulMixerIndex)
{
	//本函数返回值
	long lResult = VALUE_OK;
	//Mixer系列函数调用返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//打开第一个混音设备，没有窗口回调
	MMResult = mixerOpen(&m_HMixer, ulMixerIndex, 0, 0, MIXER_OBJECTF_MIXER);
	if (MMSYSERR_NOERROR != MMResult)
	{
		//打开失败，重置混音器句柄为NULL
		m_HMixer = NULL;
		return VALUE_MIXER_OPEN_ERROR;
	}

	MIXERCAPS stMixerCaps = { 0 };

	//获取混音器设备的能力
	MMResult = mixerGetDevCaps((UINT)m_HMixer, &stMixerCaps, sizeof(MIXERCAPS));
	if (MMSYSERR_NOERROR != MMResult)
	{
		return VALUE_MIXER_GET_CAPS_ERROR;
	}

	//混音器线路
	MIXERLINE stMixerLine = { 0 };
	//目标线路索引
	unsigned long ulDstLineIndex = 0;

	//获取WaveIn目标线路索引
	lResult = GetDstLineIndex(stMixerLine,
		MIXERLINE_COMPONENTTYPE_DST_WAVEIN, stMixerCaps.cDestinations,
		ulDstLineIndex);

	if (VALUE_OK != lResult) {
		return lResult;
	}
	//源线路索引
	unsigned long ulSrcLineIndex = 0;
	//获取Microphone源线路索引
	lResult = GetSrcLineIndex(stMixerLine,
		MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE, stMixerLine.cConnections,
		ulSrcLineIndex);

	if (VALUE_OK != lResult) {
		return lResult;
	}

	//线路控制器集
	MIXERLINECONTROLS stMixerLineCtrls = { 0 };

	stMixerLineCtrls.cbStruct = sizeof(MIXERLINECONTROLS);    //结构体大小
	stMixerLineCtrls.dwLineID = stMixerLine.dwLineID;
	stMixerLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;  //音量控制器
	stMixerLineCtrls.cControls = stMixerLine.cControls;
	stMixerLineCtrls.cbmxctrl = sizeof(MIXERCONTROL);     //输出缓存大小
	stMixerLineCtrls.pamxctrl = &m_stMixerCtrl;           //输出

	//获取Microphone源线路音量控制器
	MMResult = mixerGetLineControls((HMIXEROBJ)m_HMixer, &stMixerLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);

	if (MMSYSERR_NOERROR != MMResult) {
		return VALUE_MIXER_GET_LINE_CONTROL_ERROR;
	}
	return VALUE_OK;
}

/******************************************************************************
Function:    Close
Description: 关闭混音器
Return:      错误码
******************************************************************************/
long CMixerVolume::Close()
{
	//检测是否打开混音期
	if (!IsOpen()) {
		return VALUE_MIXER_ALREADY_CLOSE_ERROR;
	}

	//本函数返回值
	long lResult = VALUE_OK;
	//Mixer系列函数调用返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//关闭混音器设备
	MMResult = mixerClose(m_HMixer);

	if (MMSYSERR_NOERROR != MMResult) {
		lResult = VALUE_MIXER_CLOSE_ERROR;
	}

	m_HMixer = NULL;
	return lResult;
}

/******************************************************************************
Function:    IsOpen
Description: 混音器是否打开
Return:      错误码
******************************************************************************/
BOOL CMixerVolume::IsOpen() const
{
	//默认为未打开
	BOOL bResult = FALSE;

	//句柄非空，则认为已打开
	if (NULL != m_HMixer) {
		bResult = TRUE;
	}

	return bResult;
}

/******************************************************************************
Function:    GetValue
Description: 获取值大小
lValue: 值大小，百分点表示
Return:      错误码
******************************************************************************/
long CMixerVolume::GetValue(long& lValue)
{
	//检测是否打开混音期
	if (!IsOpen()) {
		return VALUE_MIXER_OPEN_ERROR;
	}

	//本函数返回值
	long lResult = VALUE_OK;
	//Mixer系列函数调用返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//构造参数
	MIXERCONTROLDETAILS_SIGNED stDetail = { 0 };  //值类型：有符号类型
	//混音控制器结构
	MIXERCONTROLDETAILS stMixerCtrlDetail = { 0 };
	SetMixerControlDetails(stDetail, stMixerCtrlDetail);

	//获取Microphone源线路控制器信息
	MMResult = mixerGetControlDetails((HMIXEROBJ)m_HMixer, &stMixerCtrlDetail, MIXER_GETCONTROLDETAILSF_VALUE);

	if (MMSYSERR_NOERROR != MMResult) {
		return VALUE_MIXER_GET_LINE_DETAIL_ERROR;
	}

	//转化为百分点
	lValue = stDetail.lValue * 100 / (long)m_stMixerCtrl.Bounds.dwMaximum;
	return lResult;
}

/******************************************************************************
Function:    SetValue
Description: 设置值大小
lValue:  值大小
Return:      错误码
******************************************************************************/
long CMixerVolume::SetValue(const long lValue)
{
	//参数检查
	if ((AUDIO_VOLUME_MIN > lValue) || (AUDIO_VOLUME_MAX < lValue)) {
		return VALUE_PARAM_ERROR;
	}

	//检测是否打开混音期
	if (!IsOpen()) {
		return VALUE_MIXER_OPEN_ERROR;
	}

	//本函数返回值
	long lResult = VALUE_OK;
	//Mixer系列函数调用返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//构造参数
	MIXERCONTROLDETAILS_SIGNED stDetail = { 0 };  //值类型：有符号类型
	//音量大小，百分点转为实际值
	stDetail.lValue = lValue * (long)m_stMixerCtrl.Bounds.dwMaximum / 100;

	//混音控制器结构
	MIXERCONTROLDETAILS stMixerCtrlDetail = { 0 };

	//BEGIN V100R001C05 MOD 2010-01-25 zhaoyuzhen z00137994 for 消除代码重复度
	SetMixerControlDetails(stDetail, stMixerCtrlDetail);
	//END V100R001C05 MOD 2010-01-25 zhaoyuzhen z00137994 for 消除代码重复度

	//设置Microphone源线路控制器信息
	MMResult = mixerSetControlDetails((HMIXEROBJ)m_HMixer, &stMixerCtrlDetail,
		MIXER_SETCONTROLDETAILSF_VALUE);

	if (MMSYSERR_NOERROR != MMResult)
	{
		return VALUE_MIXER_SET_LINE_DETAIL_ERROR;
	}
	return lResult;
}

/******************************************************************************
Function:    RegisterCallBack
Description: 注册处理麦克风音量改变的回调函数
pCallBackInfo:  回调函数指针
Return:      错误码
******************************************************************************/
long CMixerVolume::RegisterCallBack(const CALLBACKFUNINFO* pCallBackInfo) const
{
	//待实现
	if (NULL == pCallBackInfo) {
		return VALUE_OK;
	}
	return VALUE_OK;
}

/******************************************************************************
Function:    GetDstLineIndex
Description: 获取目标线路索引
stMixerLine:    混音器线路结构体
dwLineType:     线路类型
cDests:         混音器目标个数
ulIndex:        目标线路索引
Return:      错误码
******************************************************************************/
long CMixerVolume::GetDstLineIndex(MIXERLINE& stMixerLine,
	const DWORD dwLineType, const UINT cDests, unsigned long& ulIndex)
{
	//本函数返回值
	long lResult = VALUE_MIXER_LINE_NO_FOUND_ERROR;
	//Mixer系列函数返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//遍历混音器的所有目标线路
	for (unsigned long i = 0; i < cDests; i++)
	{
		//调用mixerGetLineInfo必须设置的几个值
		stMixerLine.cbStruct = sizeof(MIXERLINE);   //结构体大小
		stMixerLine.dwSource = 0;                   //必须置0
		stMixerLine.dwDestination = i;              //遍历目标索引

		//获取音频目标线路信息
		MMResult = mixerGetLineInfo((HMIXEROBJ)m_HMixer, &stMixerLine,
			MIXER_GETLINEINFOF_DESTINATION);

		//出错则继续
		if (MMSYSERR_NOERROR != MMResult) {
			continue;
		}
		//找到指定类型的目标线路
		if (dwLineType == stMixerLine.dwComponentType) {
			//设置线路索引
			ulIndex = i;
			//设置返回值
			lResult = VALUE_OK;
			break;
		}
	}
	return lResult;
}

/******************************************************************************
Function:    GetDstLineIndex
Description: 获取源线路索引
stMixerLine:    混音器线路结构体
dwLineType:     线路类型
cConns:         给定目标线路的连接个数
ulIndex:         源线路索引
Return:      错误码
******************************************************************************/
long CMixerVolume::GetSrcLineIndex(MIXERLINE& stMixerLine,
	const DWORD dwLineType, const UINT cConns, unsigned long& ulIndex)
{
	//本函数返回值
	long lResult = VALUE_MIXER_LINE_NO_FOUND_ERROR;
	//Mixer系列函数返回值
	MMRESULT MMResult = MMSYSERR_NOERROR;

	//遍历混音器给定目标线路的所有连接
	for (unsigned long i = 0; i < cConns; i++)
	{
		//调用mixerGetLineInfo必须设置的几个值
		stMixerLine.cbStruct = sizeof(MIXERLINE);   //结构体大小
		stMixerLine.dwSource = i;                   //遍历源索引
		stMixerLine.dwDestination = stMixerLine.dwDestination;  //指定目标索引

		//获取源线路信息
		MMResult = mixerGetLineInfo((HMIXEROBJ)m_HMixer, &stMixerLine,
			MIXER_GETLINEINFOF_SOURCE);

		//出错则继续
		if (MMSYSERR_NOERROR != MMResult) {
			continue;
		}
		//找到指定类型的源线路
		if (dwLineType == stMixerLine.dwComponentType) {
			//设置线路索引
			ulIndex = i;
			//设置返回值
			lResult = VALUE_OK;
			break;
		}
	}
	return lResult;
}

/******************************************************************************
Function:    GetDstLineIndex
Description: 获取源线路索引
Input:       MIXERCONTROLDETAILS_SIGNED stDetail   : 构造参数
MIXERCONTROLDETAILS stMixerCtrlDetail : 混音控制器结构体
Output:      MIXERCONTROLDETAILS stMixerCtrlDetail : 混音控制器结构体
Return:      错误码
******************************************************************************/
void CMixerVolume::SetMixerControlDetails(MIXERCONTROLDETAILS_SIGNED& stDetail,
	MIXERCONTROLDETAILS& stMixerCtrlDetail)
{
	//设置混音控制器结构体信息
	stMixerCtrlDetail.cbStruct = sizeof(MIXERCONTROLDETAILS);  //本身长度
	stMixerCtrlDetail.dwControlID = m_stMixerCtrl.dwControlID;    //控制器ID
	stMixerCtrlDetail.cbDetails = sizeof(MIXERCONTROLDETAILS_SIGNED);   //值类型
	stMixerCtrlDetail.paDetails = &stDetail;    //输出缓存
	stMixerCtrlDetail.cChannels = 1;            //通道数设为1
	stMixerCtrlDetail.cMultipleItems = 0;            //无乘项
	return;
}


AudioCollect& AudioCollect::GetInstance()
{
	static std::shared_ptr<AudioCollect> instance(new AudioCollect());
	static AudioCollect& s_instance_ref = *instance;
	return s_instance_ref;
}

AudioCollect::AudioCollect()
{
	m_lWaveInState = AUDIO_STATE_INIT_NO;  //录音设备状态
	m_lMixerState = AUDIO_STATE_INIT_NO;  //混音设备状态
	m_lVolume = AUDIO_VOLUME_MAX / 2;             //麦克风音量

	m_pobjWaveIn = NULL; //录音设备对象
	m_pobjMixerVolume = NULL; //音量控制器
	m_pAudioData = NULL;
}

AudioCollect::~AudioCollect()
{
	m_pobjWaveIn = NULL; //录音设备对象
	m_pobjMixerVolume = NULL; //音量控制器
	if (m_pAudioData) {
		AS_DELETE(m_pAudioData, MAX_AUDIO_LEN);
	}
}

/******************************************************************************
Function:    Init
Description: 初始化反向音频输入
pParam: INITPARAMINFO结构体指针
Return:      错误码
******************************************************************************/
long AudioCollect::Init()
{
	long lResult = VALUE_OK;    //本函数返回值

	if (VALUE_OK == CheckIsInited()) {
		return lResult;
	}
		
	//创建录音设备对象
	if (NULL == AS_NEW(m_pobjWaveIn)) {
		return VALUE_MEMORY_ALLOC_ERROR;
	}

	//初始化录音设备
	lResult = m_pobjWaveIn->Init(WAVEIN_BUFFER_QUEUE_SIZE);
	if (VALUE_OK != lResult) {
		//每个出口都要释放内存
		AS_DELETE(m_pobjWaveIn);

		//如果录音设备不存在，则将状态置为无法使用
		if (VALUE_DEVICE_NOT_EXIST == lResult) {
			m_lWaveInState = AUDIO_STATE_CANNOT_USE;
		}
		//置为未初始化状态
		else {
			m_lWaveInState = AUDIO_STATE_INIT_NO;
		}
		return lResult;
	}

	//创建混音器设备对象
	if (NULL == AS_NEW(m_pobjMixerVolume)) {
		return VALUE_MEMORY_ALLOC_ERROR;
	}

	//初始化混音器设备
	lResult = m_pobjMixerVolume->Init();
	if (VALUE_OK != lResult) {
		//每个出口都要释放内存
		AS_DELETE(m_pobjMixerVolume);

		//如果混音器设备不存在，则将状态置为无法使用
		if (VALUE_NO_MIXER_ERROR == lResult) {
			m_lMixerState = AUDIO_STATE_CANNOT_USE;
		}
		//置为未初始化状态
		else {
			m_lMixerState = AUDIO_STATE_INIT_NO;
		}
		return lResult;
	}

	//构造回调函数信息结构
	CALLBACKFUNINFO stCallBackFunInfo = { 0 };

	//设置回调函数信息
	stCallBackFunInfo.pCallBackFun = HandleUnifyCallBack;
	stCallBackFunInfo.pUser = this;
	//	stCallBackFunInfo.ulCallbackType = FUNTYPE_INTERFACE_REPORT_UNIFY;

	//注册音频数据通知回调
	lResult = m_pobjWaveIn->RegisterCallBack(&stCallBackFunInfo);
	if (VALUE_OK != lResult) {
		return VALUE_CALLBACK_REGISTER_ERROR;
	}

	AS_NEW(m_pAudioData, MAX_AUDIO_LEN);
	ResetData();

	//设置状态为成功初始化
	m_lWaveInState = AUDIO_STATE_INIT_ALREADY;
	m_lMixerState = AUDIO_STATE_INIT_ALREADY;
	return VALUE_OK;
}

/******************************************************************************
Function:    Release
Description: 释放反向音频输入
Return:      错误码
******************************************************************************/
long AudioCollect::Release()
{
	long lReturn = VALUE_OK;    //本函数返回值
	long lResult = VALUE_OK;    //调用其它函数返回值

	//释放录音设备
	if (NULL != m_pobjWaveIn) {
		lResult = m_pobjWaveIn->Release();

		if (VALUE_OK != lResult) {
			lReturn = lResult;
		}
		//释放内存
		AS_DELETE(m_pobjWaveIn);
	}

	//释放混音器设备
	if (NULL != m_pobjMixerVolume) {
		lResult = m_pobjMixerVolume->Release();

		if (VALUE_OK != lResult) {
			lReturn = lResult;
		}
		//释放内存
		AS_DELETE(m_pobjMixerVolume);
	}

	//如果为已初始化状态，则将其置为未初始化状态。主要考虑还有无法使用状态
	if (AUDIO_STATE_INIT_ALREADY == m_lWaveInState) {
		m_lWaveInState = AUDIO_STATE_INIT_NO;
	}

	if (AUDIO_STATE_INIT_ALREADY == m_lMixerState) {
		m_lMixerState = AUDIO_STATE_INIT_NO;
	}
	return lReturn;
}

/******************************************************************************
Function:    StartIngress
Description: 开始反向音频输入
pBusiness:  业务对象指针
Return:      错误码
******************************************************************************/
long AudioCollect::startIngress()
{
	//初始化返回值
	long lResult = VALUE_OK;    //返回值

	//必须处于已初始化状态
	lResult = CheckIsInited();
	if (VALUE_OK != lResult) {
		return lResult;
	}

	//打开录音设备，并开始录音
	lResult = m_pobjWaveIn->Open();
	if (VALUE_OK != lResult) {
		return lResult;
	}

	//必须处于已初始化状态，且混音器设备对象指针不为空
	if ((AUDIO_STATE_INIT_ALREADY == m_lMixerState) && (NULL != m_pobjMixerVolume)) {
		//忽略部分声卡设置音量的失败

		//打开混音器设备
		(void)m_pobjMixerVolume->Open();
	}
	return lResult;
}

/******************************************************************************
Function:    StopIngress
Description: 停止反向音频输入
Return:      错误码
******************************************************************************/
long AudioCollect::stopIngress()
{
	//初始化返回值
	long lResult = VALUE_OK;    //返回值

	//必须处于已初始化状态
	lResult = CheckIsInited();
	if (VALUE_OK != lResult) {
		return lResult;
	}
	
	//停止录音，并关闭录音设备
	lResult = m_pobjWaveIn->Close();
	if (VALUE_OK != lResult) {

	}

	//必须处于已初始化状态，且混音器设备对象指针不为空
	if ((AUDIO_STATE_INIT_ALREADY == m_lMixerState) && (NULL != m_pobjMixerVolume)) {
		//打开混音器设备
		lResult = m_pobjMixerVolume->Close();
		if (VALUE_OK != lResult) {

		}
	}
	return lResult;
}

/******************************************************************************
Function:    GetState
Description: 获取语音输入实例状态
Return:      实例状态：0，尚未初始化；1，成功初始化；2，无法使用；
******************************************************************************/
long AudioCollect::GetState()
{
	//直接返回录音设备状态
	return m_lWaveInState;
}

/******************************************************************************
Function:    SetVolume
Description: 设置麦克风音量
lVolume:    音量的值，范围0-100, 0 为关闭，100 为最大音量
Return:      错误码
******************************************************************************/
long AudioCollect::SetVolume(long lVolume)
{
	long lResult = VALUE_OK;    //返回值

	//必须处于已初始化状态，且混音器设备对象指针不为空
	if ((AUDIO_STATE_INIT_ALREADY == m_lMixerState) && (NULL != m_pobjMixerVolume))
	{
		//打开混音器设备
		lResult = m_pobjMixerVolume->SetValue(lVolume);
		if (VALUE_OK != lResult)
		{
		}
	}
	return lResult;
}

/******************************************************************************
Function:    GetVolume
Description: 获取麦克风音量
lVolume:    音量的值，范围0-100, 0 为关闭，100 为最大音量
Return:      错误码
******************************************************************************/
long AudioCollect::GetVolume(long& lVolume)
{
	long lResult = VALUE_OK;    //返回值

	//必须处于已初始化状态，且混音器设备对象指针不为空
	if ((AUDIO_STATE_INIT_ALREADY == m_lMixerState) && (NULL != m_pobjMixerVolume))
	{
		//打开混音器设备
		lResult = m_pobjMixerVolume->GetValue(lVolume);
		if (VALUE_OK != lResult) {

		}
	}
	return lResult;
}

/******************************************************************************
Function:    UnifyCallBack
Description: 统一回调调用接口
Input:       pParaInfo:  回调信息
pUserData:  用户数据
Return:      错误码
******************************************************************************/
long __stdcall AudioCollect::HandleUnifyCallBack(void* pUserData, unsigned long ulLen, void* pUser)
{
	long lResult = VALUE_OK;

	//参数检查
	if (NULL == pUserData || ulLen <= sizeof SVS_MEDIA_FRAME_HEADER) {
		return VALUE_PARAM_ERROR;
	}

	AudioCollect* pThis = AS_CAST<AudioCollect>(pUser);
	if (NULL == pThis) {
		return VALUE_PARAM_ERROR;
	}

	std::unique_lock<std::mutex> lck(pThis->m_mutex);
	if (pThis->m_pAudioData) {
		pThis->ResetData();
		memcpy(pThis->m_pAudioData, pUserData, ulLen);
		pThis->m_ulAudioLen = ulLen;
	}
	//只处理CUMW_NOTIFY_TYPE_IEGRESS_DATA
	//lResult = pThis->HandleNotifyCommon(*pParaInfo);
	return lResult;
}

void AudioCollect::ResetData()
{
	if (m_pAudioData) {
		memset(m_pAudioData, 0, MAX_AUDIO_LEN);
	}
}

/******************************************************************************
Function:    GetAudioFormat
Description: 获取音频格式
stInfo:     音频格式信息
Return:      错误码
******************************************************************************/
long AudioCollect::GetAudioFormat(AUDIO_FORMAT_INFO& stInfo)
{
	//必须处于已初始化状态
	long lResult = CheckIsInited();
	if (VALUE_OK != lResult) {
		return lResult;
	}
	//直接返回混音器的音频格式
	return m_pobjWaveIn->GetAudioFormat(stInfo);
}

/******************************************************************************
Function:    CheckIsInited
Description: 检查是否已经初始化
Return:      错误码
******************************************************************************/
long AudioCollect::CheckIsInited()
{
	//检查是否处于已初始化状态
	if (AUDIO_STATE_INIT_ALREADY != m_lWaveInState) {
		return VALUE_NO_INIT_ERROR;
	}

	//检查录音设备对象指针是否为空
	if (NULL == m_pobjWaveIn) {
		return VALUE_MEMORY_PTR_ERROR;
	}
	return VALUE_OK;
}

long AudioCollect::GetAudioData(unsigned char* pData, unsigned int* ulLen)
{
	std::unique_lock<std::mutex> lck(m_mutex);
	if (pData && m_pAudioData && m_ulAudioLen > 0 && m_ulAudioLen < MAX_AUDIO_LEN) {
		memcpy(pData, m_pAudioData, m_ulAudioLen);
		*ulLen = m_ulAudioLen;
		m_ulAudioLen = 0;
	}
	return VALUE_OK;
}

#endif // #ifdef _WIN32  