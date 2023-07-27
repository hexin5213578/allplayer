#include "allplayer_manager.h"
#include "avs_player_factory.h"
#include "avs_player.h"

CAllplayerManager* CAllplayerManager::GetInstance()
{
	static CAllplayerManager playerManager;
	return &playerManager;
}

CAllplayerManager::CAllplayerManager()
{

}

CAllplayerManager::~CAllplayerManager()
{

}

int32_t CAllplayerManager::parseBusinessJson(char* pBusinessJson, BusinessInfoStruct& info)
{
	AS_LOG(AS_LOG_DEBUG, "jsion text is[%s]", pBusinessJson);
	int32_t iRet = 0;
	cJSON* root = cJSON_Parse(pBusinessJson);
	if (!root) {
		AS_LOG(AS_LOG_ERROR, "parse jsion test root is NULL");
		return AS_ERROR_CODE_FAIL;
	}

	cJSON* jBusinessType = cJSON_GetObjectItem(root, "businessType");
	if (!jBusinessType) {
		AS_LOG(AS_LOG_ERROR, "input date faile jBusinessType is null");
		return AS_ERROR_CODE_FAIL;
	}
	if (!jBusinessType->valuestring) {
		AS_LOG(AS_LOG_ERROR, "input date faile jBusinessType->valueint is null");
		return AS_ERROR_CODE_FAIL;
	}

	int32_t m_BusinessType = (atoi)(jBusinessType->valuestring);
	cJSON *jAVCodecID = nullptr, *jWindowsHandle = nullptr, *jRtspurl = nullptr;
	cJSON *jdigitalHandle = nullptr,*jScale = nullptr,*jStart = nullptr;
	cJSON* jVolume = nullptr, *jSnapPath = nullptr;
	cJSON *jZoomTop = nullptr, *jZoomRight = nullptr, *jZoomBottom = nullptr, *jZoomLeft = nullptr;

	info.BussinessType = m_BusinessType;

	switch (m_BusinessType) {
	case TYPE_REALVIDEO_START:
	case TYPE_NETRECORD_START:
		
		jWindowsHandle = cJSON_GetObjectItem(root, "windowsHandle");
		if (!jWindowsHandle) {
			AS_LOG(AS_LOG_ERROR, "input date faile jWindowsHandle is null");
			return AS_ERROR_CODE_FAIL;
		}
		if (!jWindowsHandle->valuestring) {
			AS_LOG(AS_LOG_ERROR, "input date faile jWindowsHandle->valuestring is null");
			return AS_ERROR_CODE_FAIL;
		}

		jRtspurl = cJSON_GetObjectItem(root, "rtspurl");
		if (!jRtspurl) {
			AS_LOG(AS_LOG_ERROR, "input date faile jRtspurl is null");
			return AS_ERROR_CODE_FAIL;
		}
		if (!jRtspurl->valuestring) {
			AS_LOG(AS_LOG_ERROR, "input date faile jRtspurl->valuestring is null");
			return AS_ERROR_CODE_FAIL;
		}
		info.Url = string(jRtspurl->valuestring);
		info.WindowsHandle = (void*)atoi(jWindowsHandle->valuestring);
		AS_LOG(AS_LOG_DEBUG, "the jsion test from remote is [%d] [%s]", info.WindowsHandle, info.Url.c_str());
		break;

	case TYPE_DIGITAL_ZOOM:
		info.BussinessType = (atoi)(jBusinessType->valuestring);
		jdigitalHandle = cJSON_GetObjectItem(root, "DIGITALZOOM");
		if (!jdigitalHandle) {
			AS_LOG(AS_LOG_ERROR, "input date faile jdigitalHandle is null");
			return AS_ERROR_CODE_FAIL;
		}
		info.DigitalWindowsHandle = (void*)atoi(jdigitalHandle->valuestring);

		jZoomTop = cJSON_GetObjectItem(root, "top");
		if (!jZoomTop)
		{
			AS_LOG(AS_LOG_ERROR, "input date faile zoom top is null");
			return AS_ERROR_CODE_FAIL;
		}
		info.PartialTop = jZoomTop->valueint;

		jZoomRight = cJSON_GetObjectItem(root, "right");
		if (!jZoomRight)
		{
			AS_LOG(AS_LOG_ERROR, "input date faile zoom right is null");
			return AS_ERROR_CODE_FAIL;
		}
		info.PartialRight = jZoomRight->valueint;

		jZoomBottom = cJSON_GetObjectItem(root, "bottom");
		if (!jZoomBottom)
		{
			AS_LOG(AS_LOG_ERROR, "input date faile zoom bottom is null");
			return AS_ERROR_CODE_FAIL;
		}
		info.PartialBottom = jZoomBottom->valueint;

		jZoomLeft = cJSON_GetObjectItem(root, "left");
		if (!jZoomLeft)
		{
			AS_LOG(AS_LOG_ERROR, "input date faile zoom left is null");
			return AS_ERROR_CODE_FAIL;
		}
		info.PartialLeft = jZoomLeft->valueint;
		break;
		
	case TYPE_NETRECORD_CONTROL:
		jScale = cJSON_GetObjectItem(root, "scale");
		if (jScale && jScale->valuestring) {
			info.Scale = (atof)(jScale->valuestring);
		}

		jStart = cJSON_GetObjectItem(root, "start");
		if (jStart && jStart->valuestring) {
			info.Start = (atof)(jStart->valuestring);
		}	
		break;

	case TYPE_VOLUME_CONTROL:
		jVolume = cJSON_GetObjectItem(root, "volume");
		info.VolumeControl = (jVolume && jVolume->valuestring) ? (atoi)(jVolume->valuestring):0;
		break;
	case TYPE_REALVIDEO_CAPTURE:
		jSnapPath = cJSON_GetObjectItem(root, "snapPath");
		info.SnapPath = (jSnapPath && jSnapPath->valuestring) ? string(jSnapPath->valuestring):"";
		break;

	case TYPE_LOCAL_RECORD_START:
		info.MP4CutFormat = 0;
		info.MP4Cutduration = 1;
		info.CameraId = "xxx";
		info.RecordPath = "D:\\";
		break;
	default:
		break;
	}
	return 0;
}

int32_t CAllplayerManager::allplayerInit(char* pLogPath, int32_t iLogLevel)
{
	return avs_player_factory::GetInstance()->init(STREAM_RECV_THREAD_DEFAULT, PLAYER_MAX_DEFAULT, pLogPath, iLogLevel);
}

int32_t CAllplayerManager::allplayerRelease()
{
	avs_player_factory::GetInstance()->release();
	avs_player_factory::GetInstance()->stop_log();
	return 0;
}

int32_t CAllplayerManager::excuteBusiness(long lBusinessID, char* pBusinessParameters)
{
	BusinessInfoStruct businessInfo;
	int iRes = parseBusinessJson(pBusinessParameters, businessInfo);
	if (iRes) {
		AS_LOG(AS_LOG_ERROR, "parase jsion file failed");
		return AS_ERROR_CODE_FAIL;
	}

	return excuteBusiness(lBusinessID, businessInfo);
}

int32_t CAllplayerManager::excuteBusiness(long lBusinessID, BusinessInfoStruct& businessInfo)
{
	avs_player* pPlayer = nullptr, * prePlayer = nullptr;

	m_concreteEgress.m_bussinessId = lBusinessID;
	switch (businessInfo.BussinessType)
	{
	case TYPE_REALVIDEO_START:
	case TYPE_NETRECORD_START:

		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		//根据业务类型判断是否需要新建avs_player对象
		if (!pPlayer) {
			pPlayer = avs_player_factory::GetInstance()->createPlayer(lBusinessID);

			m_concreteEgress.m_hWnd = businessInfo.WindowsHandle;
			m_concreteEgress.m_bussinesType = businessInfo.BussinessType;
			m_concreteEgress.m_volume = businessInfo.VolumeControl;
			m_concreteEgress.m_url = businessInfo.Url;

			//绑定业务数据
			auto iRes = pPlayer->init(&m_concreteEgress);
			if (AS_ERROR_CODE_FAIL == iRes) {
				AS_LOG(AS_LOG_ERROR, "avs_player init faile");
				return AS_ERROR_CODE_FAIL;
			}
		}
		if (AS_ERROR_CODE_OK != pPlayer->play()) {
			AS_LOG(AS_LOG_ERROR, "avs_player realtime play faile");
			return AS_ERROR_CODE_FAIL;
		}
		break;

	case TYPE_REALVIDEO_STOP:
	case TYPE_NETRECORD_STOP:

		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			AS_LOG(AS_LOG_ERROR, "realtim stop ,not find avs_player");
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->stop();
		avs_player_factory::GetInstance()->deletePlayer(lBusinessID);
		break;

	case TYPE_NETRECORD_CONTROL:

		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			AS_LOG(AS_LOG_ERROR, "realtim stop ,not find avs_player");
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->vcr_control(businessInfo.Start, businessInfo.Scale);
		break;

	case TYPE_REALVIDEO_CAPTURE:

		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			AS_LOG(AS_LOG_ERROR, "before snap picture ,not find avs_player");
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->snap_picture(businessInfo.SnapPath);
		break;

	case TYPE_DIGITAL_ZOOM:

		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			AS_LOG(AS_LOG_ERROR, "vedio digital zoom ,not find avs_player");
			return AS_ERROR_CODE_FAIL;
		}
		if (!businessInfo.DigitalWindowsHandle) {
			AS_LOG(AS_LOG_ERROR, "digital zoom windows handle is NULL");
			return AS_ERROR_CODE_FAIL;
		}
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
		pPlayer->digital_zoom(businessInfo);
#endif
		break;

	case TYPE_DIGITAL_ZOOM_IN:
		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			AS_LOG(AS_LOG_ERROR, "vedio digital zoom ,not find avs_player");
			return AS_ERROR_CODE_FAIL;
		}
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
		pPlayer->digital_zoom_in();
#endif
		break;

	case TYPE_LOCAL_RECORD_START:
		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			AS_LOG(AS_LOG_ERROR, "local record fail,not find avs_player");
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->start_record(businessInfo);
		break;

	case TYPE_LOCAL_RECORD_STOP:

		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			AS_LOG(AS_LOG_ERROR, "local record fail,not find avs_player");
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->stop_record();
		break;

	case TYPE_VOLUME_CONTROL:

		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->set_volume(businessInfo.VolumeControl);
		break;

	case TYPE_NETRECORD_PAUSE:
		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->pause();
		break;

	case TYPE_NETRECORD_RESUME:
		pPlayer = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
		if (!pPlayer) {
			return AS_ERROR_CODE_FAIL;
		}
		pPlayer->resume();
		break;

	default:
		break;
	}
	return AS_ERROR_CODE_OK;
}

void CAllplayerManager::registerStatusCallback(allplay_status_callback pCallBack)
{
	avs_player_factory::GetInstance()->registerStatusCallback(pCallBack);
}

void CAllplayerManager::registerDataCallback(allplay_progress_callback pCallBack)
{
	avs_player_factory::GetInstance()->registerDataCallback(pCallBack);
}
