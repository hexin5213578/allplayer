#include "allplayer_manager.h"
#include "avs_player_factory.h"
#include "avs_player.h"
#include "avs_voice_client.h"
#include "view_factory.h"
#include "socks_config.h"
#include "avs_watermark.h"
#include "avs_ingress.h"
#include <thread>
#include <string>

extern int kULimit;

using std::string;

static cJSON* GetJsonObjectItem(cJSON* root, const char* string, bool required = true)
{
	cJSON* jRet = nullptr;
	jRet = cJSON_GetObjectItem(root, string);
	if (!jRet || (jRet && !jRet->valuestring)) {
		if (required) {
			AS_LOG(AS_LOG_ERROR, "input date fail: %s is null", string);
		}
		return nullptr;
	}
	return jRet;
}

static int32_t GetJsonObjectItemInt(cJSON* root, const char* string, bool required = true)
{
	cJSON* jRet = nullptr;
	jRet = cJSON_GetObjectItem(root, string);
	if (!jRet) {
		if (required) {
			AS_LOG(AS_LOG_ERROR, "input date fail: %s is null", string);
		}
		return -1;
	}
	return jRet->valueint;
}

static AvsPlayer* FindAvsPlayerByBizId(long bizId, const char* des, bool needed = true)
{
	AvsPlayer* player = avs_player_factory::GetInstance()->findPlayerByBusinessID(bizId);
	if (nullptr == player && needed) {
		AS_LOG(AS_LOG_ERROR, "excute %s fail: couldn't find avs_player %ld", des, bizId);
	}
	return player;
}

#define FIND_PLAYER_RET(bizId, des)	\
	player = FindAvsPlayerByBizId(bizId, des);	\
	if(nullptr == player)						\
		return AS_ERROR_CODE_FAIL;				\

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

#define GetJsonItemStringRequired(json, root, string)	\
	json = GetJsonObjectItem(root, string, true);	\
	if(!json)	{						\
		ret = AS_ERROR_CODE_FAIL;		\
		break;							\
	}									\

		
int static generateDumpStruct(FileDumpSt& dump, cJSON* root) {
	int ret = 0;
	auto path = GetJsonObjectItem(root, "RecordDownloadPath");
	auto cutFormat = GetJsonObjectItem(root, "DownloadCutFormat");
	if (!path || !cutFormat) {
		ret = AS_ERROR_CODE_FAIL;
		return ret;
	}
	dump.filePath = string(path->valuestring);
	dump.cutFormat = (atoi)(cutFormat->valuestring);
	
	auto fileFormat = GetJsonObjectItemInt(root, "FileFormat", false);
	if (fileFormat >= 0 && fileFormat < FileDumpSt::KMax) {
		dump.fileFormat = (FileDumpSt::formatEnum)(fileFormat);
	}
	else {
		dump.fileFormat = FileDumpSt::kMP4;
	}

#ifdef _DEBUG
	//dump.fileFormat = FileDumpSt::kAVS;
#endif

	if (FileDumpSt::kAVS == dump.fileFormat) {
		/*auto jkey = GetJsonObjectItem(root, "EncryptKey", false);
		if (!jkey) {
			dump.key = makeRandStr(32);
		}
		else {
			auto szKey = std::string(jkey->valuestring);
			if(szKey.length() < 16) {
				dump.key = makeRandStr(32);
			}
			else if (szKey.length() > 32) {
				dump.key = szKey.substr(0, 32);
			}
			else {
				dump.key = szKey + szKey.substr(0, 32 - szKey.length());
			}
		}*/
		dump.key = makeRandStr(32);
		auto jLimitDays = GetJsonObjectItemInt(root, "LimitDays", false);
		dump.limit_days = jLimitDays >= 0 ? jLimitDays : kULimit;

		auto jLimitTimes = GetJsonObjectItemInt(root, "LimitTimes", false);
		dump.limit_times = jLimitTimes >= 0 ? jLimitTimes : kULimit;
	}

#ifdef _DEBUG
	dump.limit_days = kULimit;
	dump.limit_times = kULimit;
#endif

	//Fixxx: 该参数没有在play时生效
	cJSON* jScale = GetJsonObjectItem(root, "DownloadScale", false);
	dump.scale = jScale ? (atof)(jScale->valuestring) : 1.0;
	
	auto cutDuration = GetJsonObjectItem(root, "DownloadCutDuration", false);
	auto cutSize = GetJsonObjectItem(root, "DownloadCutSize", false);
	if (FileDumpSt::kSize == dump.cutFormat) {
		dump.cutSize = cutSize ? (atoi)(cutSize->valuestring) : 50;
	}
	else if (FileDumpSt::kTime == dump.cutFormat) {
		dump.cutDuration = cutDuration ? (atoi)(cutDuration->valuestring) : 10;
	}
	else {
		ret = AS_ERROR_CODE_FAIL;
	}
	return ret;
}

int32_t CAllplayerManager::parseBussinessJson(long lBusinessID, char* pBusinessJson, BizParams& info, char* pResultData, int len)
{
	AS_LOG(AS_LOG_DEBUG, "json text: %s.", pBusinessJson);
	int32_t iRet = 0;
	cJSON* root = nullptr, *jBusinessType = nullptr;
	
	if (!(root = cJSON_Parse(pBusinessJson)) || !(jBusinessType = GetJsonObjectItem(root, "BusinessType"))) {
		AS_LOG(AS_LOG_ERROR, "parse json eror.");
		if(root) cJSON_Delete(root);
		return AS_ERROR_CODE_FAIL;
	}
		
	ExcuteInfo exInfo;
	int32_t businessType = (atoi)(jBusinessType->valuestring);
	info.BizType = businessType;
	exInfo.bizType = businessType;
	exInfo.bizId = lBusinessID;

	cJSON* jWindowsHandle = nullptr, * jUrl = nullptr, * jVolume = nullptr, * jScaleOrSpeed = nullptr;
	cJSON* jSnapPath = nullptr, * jSnapFormat = nullptr;
	cJSON* jSon = nullptr;
	//cJSON *jStartTime = nullptr, *jEndTime = nullptr;

	int ret = 0;
	switch (businessType) {
	case TYPE_SOCKS_INIT: 
	{
		cJSON* jIp = GetJsonObjectItem(root, "SocksIP");
		cJSON* jPort = GetJsonObjectItem(root, "SocksPort");
		if (!jIp || !jPort) {
			ret = AS_ERROR_CODE_FAIL;
			break;
		}
		std::string ip, username, password;
		uint16_t port;
		ip = string(jIp->valuestring);
		port = atoi(jPort->valuestring);
		
		if (cJSON* jUsername = GetJsonObjectItem(root, "SocksUsername", false)) {
			username = string(jUsername->valuestring);
		}
		
		if (cJSON* jPwd = GetJsonObjectItem(root, "SocksPassword", false)) {
			password = string(jPwd->valuestring);
		}
		SocksConfig::GetSocksConfigInstance()->setSocksInfo(ip, port, username, password);
	}
	break;

	case TYPE_SOCKS_CANCEL:
		SocksConfig::GetSocksConfigInstance()->cancelSocks();
	break;

	case TYPE_REALVIDEO_START:
	case TYPE_NETRECORD_START:
	case TYPE_MULTI_REALVIDEO_START:
	case TYPE_MULTI_RECORD_START: 
	{
		if (TYPE_MULTI_REALVIDEO_START <= businessType) {
			cJSON* jMultiFragsCount = GetJsonObjectItem(root, "MultiFragsCount");
			cJSON* jMultiWindowsHandle = cJSON_GetObjectItem(root, "MultiHWNDs");
			if (!jMultiFragsCount || !jMultiWindowsHandle) {
				ret = AS_ERROR_CODE_FAIL;
				break;
			}

			info.StitchInfo.streams = atoi(jMultiFragsCount->valuestring);
			int iArrayCnt = cJSON_GetArraySize(jMultiWindowsHandle);
			if (iArrayCnt > 0) {
				std::vector<void*> hwndVec;
				for (int i = 0; i < iArrayCnt; i++) {
					cJSON* jHWND = cJSON_GetArrayItem(jMultiWindowsHandle, i);
                    if (jHWND){
                        const char* handle_str = jHWND->valuestring;
                        uintptr_t handle_value = strtoull(handle_str, nullptr, 10);
                        void* handle = reinterpret_cast<void*>(handle_value);
                        hwndVec.push_back(handle);
                    }
				}
				info.StitchInfo.HWNDs = hwndVec;
			}
		}
		else {
            //TODO 整理代码
			jWindowsHandle = GetJsonObjectItem(root, "BusinessHwnd", false);
            const char* handle_str = jWindowsHandle->valuestring;
            uintptr_t handle_value = strtoull(handle_str, nullptr, 10);
            void* handle = reinterpret_cast<void*>(handle_value);
            if (handle) {
                info.WindowsHandle = handle;
            }else {
                info.WindowsHandle = (void*)INVALID_WND;
            }
			cJSON* jCamera = GetJsonObjectItem(root, "CameraID", false);
			if (jCamera) {
				info.CameraId = std::string(jCamera->valuestring);
			}
		}

		GetJsonItemStringRequired(jUrl, root, "RtspUrl");
		info.Url = string(jUrl->valuestring);

		if (TYPE_NETRECORD_START == businessType) {
			cJSON* jHighSpeedCache = GetJsonObjectItem(root, "HighSpeedCache", false);
			info.HighSpeedCache = jHighSpeedCache ? atoi(jHighSpeedCache->valuestring) : 0;

			jScaleOrSpeed = GetJsonObjectItem(root, "ScaleOrSpeed", false);
			VcrControllSt vcrControllSt;
			cJSON* jScale = GetJsonObjectItem(root, "PlayScale", false);
			cJSON* jStart = GetJsonObjectItem(root, "NptPos", false);
			jScaleOrSpeed = GetJsonObjectItem(root, "ScaleOrSpeed", false);
			if (jScaleOrSpeed) {
				int iScaleOrSpeed = (atoi)(jScaleOrSpeed->valuestring);
				if (0 == iScaleOrSpeed || 1 == iScaleOrSpeed) {
					vcrControllSt.scaleOrSpeed = iScaleOrSpeed;
				}
			}
			vcrControllSt.scale = jScale ? (atof)(jScale->valuestring) : 1.0;
			vcrControllSt.start = jStart ? (atof)(jStart->valuestring) : 0.0;
			info.vcrSt = vcrControllSt;
		}
		cJSON* jRealOrQuality = GetJsonObjectItem(root, "RealOrQuality", false);
		info.PlayMode = jRealOrQuality ? atoi(jRealOrQuality->valuestring): 0;
		
		if (info.PlayMode) {
			cJSON* jCacheSize = GetJsonObjectItem(root, "CacheSize", false);
			info.CacheSize = jCacheSize ? atoi(jCacheSize->valuestring) : 5;
		}
		
		cJSON* jDecodeType = GetJsonObjectItem(root, "DecodeType", false);
		info.DecodeType = jDecodeType ? (EDecodeType)atoi(jDecodeType->valuestring) : kSoft;
		
		jVolume = GetJsonObjectItem(root, "VolumeValue", false); 
		info.VolumeControl = jVolume ? atoi(jVolume->valuestring) : -1;
	}
	break;

	case TYPE_URL_START:
	{
		jWindowsHandle = GetJsonObjectItem(root, "BusinessHwnd", false);
        const char* handle_str = jWindowsHandle->valuestring;
        uintptr_t handle_value = strtoull(handle_str, nullptr, 10);
        void* handle = reinterpret_cast<void*>(handle_value);
        if (handle) {
            info.WindowsHandle = handle;
        }else {
            info.WindowsHandle = (void*)INVALID_WND;
        }

		GetJsonItemStringRequired(jUrl, root, "Url");
		info.Url = string(jUrl->valuestring);
	
		auto format = GetJsonObjectItemInt(root, "UrlFormat", false);
		info.PlayMode = (format >= 0) ? format : 0;

		cJSON* jDecodeType = GetJsonObjectItem(root, "DecodeType", false);
		info.DecodeType = jDecodeType ? (EDecodeType)atoi(jDecodeType->valuestring) : kSoft;

		jVolume = GetJsonObjectItem(root, "VolumeValue", false);
		info.VolumeControl = jVolume ? atoi(jVolume->valuestring) : -1;
	}
		break;

	case TYPE_REALVIDEO_STOP:
	case TYPE_NETRECORD_STOP:
	{
		jWindowsHandle = GetJsonObjectItem(root, "BusinessHwnd", false);
        const char* handle_str = jWindowsHandle->valuestring;
        uintptr_t handle_value = strtoull(handle_str, nullptr, 10);
        void* handle = reinterpret_cast<void*>(handle_value);
        if (handle) {
            info.WindowsHandle = handle;
        }else {
            info.WindowsHandle = (void*)INVALID_WND;
        }
	}
		break;

	case TYPE_SET_RtspUrl:
		GetJsonItemStringRequired(jUrl, root, "RtspUrl");
		info.Url = string(jUrl->valuestring);
	break;

	case TYPE_DIGITAL_START:
	case TYPE_DIGITAL_UPDATE:
	{
		cJSON* jdigitalHandle = GetJsonObjectItem(root, "PartialHwnd");
		cJSON* jZoomTop = GetJsonObjectItem(root, "PartialTop");
		cJSON* jZoomRight = GetJsonObjectItem(root, "PartialRight");
		cJSON* jZoomBottom = GetJsonObjectItem(root, "PartialBottom");
		cJSON* jZoomLeft = GetJsonObjectItem(root, "PartialLeft");
		if (!jdigitalHandle || !jZoomTop || !jZoomRight || !jZoomBottom || !jZoomLeft) {
			ret = AS_ERROR_CODE_FAIL;
			break;
		}

		ZoomSt zoomSt;
        const char* handle_str = jdigitalHandle->valuestring;
        uintptr_t handle_value = strtoull(handle_str, nullptr, 10);
        void* handle = reinterpret_cast<void*>(handle_value);
        if (handle) {
            zoomSt.zoomHwnd = handle;
        }else {
            zoomSt.zoomHwnd = (void*)INVALID_WND;
        }
		zoomSt.top = (atoi)(jZoomTop->valuestring);
		zoomSt.left = (atoi)(jZoomLeft->valuestring);
		zoomSt.right = (atoi)(jZoomRight->valuestring);
		zoomSt.bottom = (atoi)(jZoomBottom->valuestring);

		exInfo.varInfo = (void*)&zoomSt;
		ret = excuteInternal(exInfo);
	}
	break;

	case TYPE_NETRECORD_CONTROL:
	case TYPE_MULTI_RECORD_CONTROL:
	{
		VcrControllSt vcrControllSt;
		cJSON* jScale = GetJsonObjectItem(root, "PlayScale", false);
		cJSON* jStart = GetJsonObjectItem(root, "NptPos", false);
		jScaleOrSpeed = GetJsonObjectItem(root, "ScaleOrSpeed", false);
		if (jScaleOrSpeed) {
			int iScaleOrSpeed = (atoi)(jScaleOrSpeed->valuestring);
			if (0 == iScaleOrSpeed || 1 == iScaleOrSpeed) {
				vcrControllSt.scaleOrSpeed = iScaleOrSpeed;
			}
		}
		vcrControllSt.scale = jScale ? (atof)(jScale->valuestring) : 1.0;
		vcrControllSt.start = jStart ? (atof)(jStart->valuestring): -1.0;
		exInfo.varInfo = (void*)&vcrControllSt;
		ret = excuteInternal(exInfo);
	}
	break;

	case TYPE_VOLUME_CONTROL:
		jVolume = GetJsonObjectItem(root, "VolumeValue");
		info.VolumeControl = jVolume ? (atoi)(jVolume->valuestring) : 0;
	break;
	
	case TYPE_CAPTURE:
	{
		jSnapPath = GetJsonObjectItem(root, "CapturePath", false);
		jSnapFormat = GetJsonObjectItem(root, "CaptureFormat", false);
		info.SnapPath = jSnapPath ? string(jSnapPath->valuestring) : "";
		info.SnapFormat = PicFormat(jSnapFormat ? (atoi)(jSnapFormat->valuestring) : 0);
	}
	break;

	case TYPE_LOCAL_RECORD_START:
	{
		cJSON* jCameraId = GetJsonObjectItem(root, "CameraID", false);
		if (jCameraId) {
			info.CameraId = string(jCameraId->valuestring);
		}
		ret = generateDumpStruct(info.FileDump, root);
	}
	break;
	
	case TYPE_GET_MEDIA_INFO:
	case TYPE_GET_PIC_PARAMS:
	{
		if (!pResultData) {
			AS_LOG(AS_LOG_ERROR, "parse %d command fail, there is no buffer to recevie data.", businessType);
			ret = AS_ERROR_CODE_FAIL;
			break;
		}
	}
	break;

	case TYPE_DOWNLOAD_START: {
		GetJsonItemStringRequired(jUrl, root, "RtspUrl");
		auto jCameraId = GetJsonObjectItem(root, "CameraID", false);
		if (jCameraId) {
			info.CameraId = string(jCameraId->valuestring);
		}

		info.Url = string(jUrl->valuestring);	
		jScaleOrSpeed = GetJsonObjectItem(root, "ScaleOrSpeed", false);
		if (jScaleOrSpeed) {
			int iScaleOrSpeed = (atoi)(jScaleOrSpeed->valuestring);
			if (0 == iScaleOrSpeed || 1 == iScaleOrSpeed) {
				info.ScaleOrSpeed = iScaleOrSpeed;
				info.vcrSt.scaleOrSpeed = info.ScaleOrSpeed;
			}
		}
		ret = generateDumpStruct(info.FileDump, root);
	}
	break;

	case TYPE_AUDIO_TALK_START:
	case TYPE_VOICE_BROADCASR_START: {
		GetJsonItemStringRequired(jUrl, root, "RtspUrl");
		VoiceSt voice(exInfo.bizId, exInfo.bizType);
		voice.voiceUrl = string(jUrl->valuestring);
		exInfo.varInfo = (void*)&voice;
		ret = excuteInternal(exInfo);
	}
	break;
	
	case TYPE_FILE_BROADCASR_START: {
		GetJsonItemStringRequired(jUrl, root, "RtspUrl");
		cJSON* wavFile = nullptr;
		GetJsonItemStringRequired(wavFile, root, "WavPath");
		cJSON* wavLoop = GetJsonObjectItem(root, "WavLoop", false);
	
		VoiceSt voice(exInfo.bizId, exInfo.bizType);
		voice.voiceUrl = string(jUrl->valuestring);
		voice.wavFile = string(wavFile->valuestring);
		voice.loop = wavLoop ? atoi(wavLoop->valuestring) : 0;
		exInfo.varInfo = (void*)&voice;
		ret = excuteInternal(exInfo);
	}
	break;
	
	case TYPE_RENDER_TYPE: {
		auto jRenderType = GetJsonObjectItem(root, "RenderType");
		if (!jRenderType) {
			ret = AS_ERROR_CODE_FAIL;
			break;
		}
		int renderType = (atoi)(jRenderType->valuestring);
		exInfo.varInfo = (void*)&renderType;
		ret = excuteInternal(exInfo);
	}
	break;

	case TYPE_PICTURE_PARAMS: {
		std::string pictureParams = std::string(pBusinessJson);
		exInfo.varInfo = (void*)&pictureParams;
		ret = excuteInternal(exInfo);
	}
		break;

	case TYPE_URL_PROBE: {
		if (!pResultData) {
			AS_LOG(AS_LOG_ERROR, "parse %d command fail, there is no buffer to recevie data.", businessType);
			ret = AS_ERROR_CODE_FAIL;
			break;
		}

		GetJsonItemStringRequired(jUrl, root, "Url");
		auto info = avs_ingress_prober::probe_url(string(jUrl->valuestring));
		if(len > 0 && (info.size() >= len)) {
			AS_LOG(AS_LOG_ERROR, "process %d command exception, buffer is not enough to recevie data.", businessType);
		}
		if(!info.empty()) {
			strcpy(pResultData, info.data());
		}
	}
	break;
	
	case TYPE_URL_SEEK: {
		if (!(jSon = cJSON_GetObjectItem(root, "Pos"))) {
			ret = AS_ERROR_CODE_FAIL;
			break;
		}
		SeekSt seek;
		seek.position = jSon->valuedouble;
		exInfo.varInfo = (void*)&seek;
		ret = excuteInternal(exInfo);
	}
	break;

	case TYPE_URL_SPEED: {
		auto speed = cJSON_GetObjectItem(root, "Speed");
		if (!speed) {
			ret = AS_ERROR_CODE_FAIL;
			break;
		}
		VcrControllSt vcr;
		vcr.scale = speed->valuedouble;
		exInfo.varInfo = (void*)&vcr;
		ret = excuteInternal(exInfo);
	}
		break;

	case TYPE_URL_VOLUME: {
		auto volume = GetJsonObjectItemInt(root, "Volume");
		if (volume >= 0) {
			info.VolumeControl = volume;
		}
		else {
			ret = AS_ERROR_CODE_FAIL;
		}
	}
		break;
	
	case TYPE_SKIP_NOKEY_FRAME: {
		jSon = GetJsonObjectItem(root, "SkipNoKey");
		if (!jSon) {
			ret = AS_ERROR_CODE_FAIL;
			break;
		}
		uint8_t skip = atoi(jSon->valuestring);
		exInfo.varInfo = (void*)&skip;
		ret = excuteInternal(exInfo);
	}
	break;
	
	case TYPE_CAPTURE_FRAMES: {
		jSnapPath = GetJsonObjectItem(root, "CapturePath");
		jSon = GetJsonObjectItem(root, "CaptureCount");
		if (!jSon || !jSnapPath) {
			ret = AS_ERROR_CODE_FAIL;
			break;
		}
		CaptureSt captureSt(string(jSnapPath->valuestring), atoi(jSon->valuestring));
		exInfo.varInfo = (void*)&captureSt;
		ret = excuteInternal(exInfo);
	}
	break;

	case TYPE_GET_EXPERIENCE_DATA: {
		cJSON* jPlayers = cJSON_GetObjectItem(root, "BussinessIdArr");
		if (!pResultData || !jPlayers) {
			ret = AS_ERROR_CODE_PARAM;
			break;
		}
		int iArrayCnt = cJSON_GetArraySize(jPlayers);
		ExpCollector collector(iArrayCnt);
		if (iArrayCnt > 0) {
			for (int i = 0; i < iArrayCnt; i++) {
				cJSON* player = cJSON_GetArrayItem(jPlayers, i);
				collector.bizArr[i] = player ? player->valueint : -1;
			}
	
			exInfo.varInfo = (void*)&collector;
			ret = excuteInternal(exInfo, pResultData, len);
		}
	}
	break;

	case TYPE_SET_WATERMARK: {
		ret = WaterMarkManager::get_instance()->parse_configure(std::string(pBusinessJson));
		break;
	}

	default: {
		PrintI("unrecognized api");
		break;
	}
	}

	if (root) {
		cJSON_Delete(root);
	}
	return ret;
}

int32_t CAllplayerManager::allplayerInit(char* pLogPath, int32_t iLogLevel, int thdLimit) 
{
	auto ret = avs_player_factory::GetInstance()->init(STREAM_RECV_THREAD_DEFAULT, PLAYER_MAX_DEFAULT, pLogPath, iLogLevel);
	if (!ret) {
		auto cpus = std::thread::hardware_concurrency();
		if (thdLimit > cpus) {
#ifdef _WIN64
			thdLimit = cpus / 4;
#else 
			thdLimit = 0;
#endif // _WIN64
		}
		avs_player_factory::GetInstance()->setDecoderThreadLimit(thdLimit);
	}
	return ret;
}

int32_t CAllplayerManager::allplayerRelease() {
	avs_player_factory::GetInstance()->release();
	avs_player_factory::GetInstance()->stopLog();
	return 0;
}

int32_t CAllplayerManager::excuteBussiness(long lBusinessID, char* pBizParameters, char* pResultData, int len)
{
	BizParams bizParams;
	int ret = parseBussinessJson(lBusinessID, pBizParameters, bizParams, pResultData, len);
	if (ret) {
		AS_LOG(AS_LOG_ERROR, "parase json %s failed.", pBizParameters);
		return AS_ERROR_CODE_FAIL;
	}
	return excuteBussiness(lBusinessID, bizParams, pResultData, len);
}

int32_t CAllplayerManager::excuteBussiness(long lBusinessID, BizParams& bizParams, char* pResultData, int len)
{
	AvsPlayer* player = nullptr;
	int32_t ret = 0;

	bizParams.BizId = lBusinessID;
	auto type = bizParams.BizType;
	switch (type) 
	{
	case TYPE_REALVIDEO_START:
	case TYPE_NETRECORD_START:
	case TYPE_DOWNLOAD_START:
	case TYPE_URL_START:
	case TYPE_MULTI_REALVIDEO_START:
	case TYPE_MULTI_RECORD_START: {
		bool fake = false;
		if (TYPE_MULTI_REALVIDEO_START == type || TYPE_MULTI_RECORD_START == type) {
			for (auto& wnd : bizParams.StitchInfo.HWNDs) {
				if (((void*)INVALID_WND != wnd) && hwnds_.count(wnd)) {
					fake = true;
					break;
				}
			}
		}
		else if ((void*)INVALID_WND != bizParams.WindowsHandle && hwnds_.count(bizParams.WindowsHandle)) {
			fake = true;
		}

		if (fake) {
			AS_LOG(AS_LOG_ERROR, "player[%ld] reuse hwnd [%p], maybe crash, please stop previous player.", lBusinessID, bizParams.WindowsHandle);
			return AS_ERROR_CODE_INVALID;
		}

		player = FindAvsPlayerByBizId(lBusinessID, "", false);
		//根据业务类型判断是否需要新建avs_player对象
		if (!player && !(player = avs_player_factory::GetInstance()->createPlayer(bizParams))) {
			AS_LOG(AS_LOG_ERROR, "create player[%ld] failed.");
			return AS_ERROR_CODE_INVALID;
		}

		if ((ret = player->play())) {
			AS_LOG(AS_LOG_ERROR, "player[%ld] play fail.", lBusinessID);
			return ret;
		}

		auto description = player->getPlayInfo();
		if (!description.empty() && pResultData) {
			if (len <= 0 || description.length() < len) {
				strcpy(pResultData, description.data());
			}
		}

		//connected
		if (!ret) {
			if (TYPE_MULTI_REALVIDEO_START == bizParams.BizType || TYPE_MULTI_RECORD_START == bizParams.BizType) {
				for (auto& wnd : bizParams.StitchInfo.HWNDs) {
					if ((void*)INVALID_WND != wnd) {
						hwnds_.insert(wnd);
					}
				}
			}
			else {
				if ((void*)INVALID_WND != bizParams.WindowsHandle) {
					hwnds_.insert(bizParams.WindowsHandle);
				}
			}
		}
	}
		break;

	case TYPE_REALVIDEO_STOP:
	case TYPE_NETRECORD_STOP:
	case TYPE_DOWNLOAD_STOP:
	case TYPE_URL_STOP:
	case TYPE_MULTI_REALVIDEO_STOP:
	case TYPE_MULTI_RECORD_STOP:
	{
		player = FindAvsPlayerByBizId(bizParams.BizId, "stop", false);
		if (nullptr == player && ((void*)INVALID_WND != bizParams.WindowsHandle)) {
			player = avs_player_factory::GetInstance()->findPlayerByHwnd(bizParams.WindowsHandle);	
			if (player) {
				AS_LOG(AS_LOG_WARNING, "found player[%ld] by hwnd [%p].", player->id, bizParams.WindowsHandle);
			}
			else {
				AS_LOG(AS_LOG_WARNING, "cound't find player by hwnd [%p].", bizParams.WindowsHandle);
			}
		}

		if (!player) {
			AS_LOG(AS_LOG_WARNING, "stop biz[%ld] faild, not found.", bizParams.BizId);
			return AS_ERROR_CODE_FAIL;
		}

		if (TYPE_MULTI_REALVIDEO_START == bizParams.BizType || TYPE_MULTI_RECORD_START == bizParams.BizType) {
			vector<void*> hwnds = player->getHwnds();
			for (auto& wnd : hwnds) {
				if ((void*)INVALID_WND != wnd)	
					hwnds_.erase(wnd);
			}
		}
		else {
			void* hwnd = player->getHwnd();
			hwnds_.erase(hwnd);
		}
		avs_player_factory::GetInstance()->deletePlayer(player->id);
	}
		break;

	case TYPE_CAPTURE:
		FIND_PLAYER_RET(lBusinessID, "capture")
		player->snapPicture(bizParams.SnapPath, bizParams.SnapFormat);
		break;

	case TYPE_LOCAL_RECORD_START:
		FIND_PLAYER_RET(lBusinessID, "local record start")
		player->startCapture(bizParams);
		break;

	case TYPE_LOCAL_RECORD_STOP:
		FIND_PLAYER_RET(lBusinessID, "local record stop")
		player->stopCapture();
		break;

	case TYPE_REALVIDEO_PAUSE:
	case TYPE_NETRECORD_PAUSE:
	case TYPE_DOWNLOAD_PAUSE:
	case TYPE_URL_PAUSE:
	case TYPE_MULTI_RECORD_PAUSE:
		FIND_PLAYER_RET(lBusinessID, "pause")
		player->pause(true);
		break;

	case TYPE_REALVIDEO_RESUME:
	case TYPE_NETRECORD_RESUME:
	case TYPE_DOWNLOAD_CONTINUE:
	case TYPE_URL_RESUE:
	case TYPE_MULTI_RECORD_RESUME:
		FIND_PLAYER_RET(lBusinessID, "resume")
		player->pause(false);
		break;

	case TYPE_VOLUME_CONTROL:
	case TYPE_URL_VOLUME:
		FIND_PLAYER_RET(lBusinessID, "set volume")
		player->setVolume(bizParams.VolumeControl);
		break;

	case TYPE_VOLUME_GET:
		FIND_PLAYER_RET(lBusinessID, "get volume")
		return player->audio_volume;
		break;

	case TYPE_DIGITAL_STOP:
		FIND_PLAYER_RET(lBusinessID, "zoom stop")
	#ifdef _WIN32
			player->zoomIn();
	#endif	//
		break;

	case TYPE_GET_MEDIA_INFO: {
		FIND_PLAYER_RET(lBusinessID, "get media info")
		auto info = player->getMediaInfo();
		if (len > 0 && (info.size() >= len)) {
			AS_LOG(AS_LOG_WARNING, "process %d command fail, buffer is not enough to recevie data.", bizParams.BizType);
		}
		
		if (!info.empty()) {
			strcpy(pResultData, info.data());
		}
	}
		break;

	case TYPE_AUDIO_OPEN: {
		FIND_PLAYER_RET(lBusinessID, "open audio")
		player->setAudioChannel(true);
	}
		break;

	case TYPE_AUDIO_CLOSE: {
		FIND_PLAYER_RET(lBusinessID, "close audio")
		player->setAudioChannel(false);
	}
		break;
	
	case TYPE_AUDIO_TALK_STOP:
	case TYPE_VOICE_BROADCASR_STOP:
	case TYPE_FILE_BROADCASR_STOP:
		voice_client_manager::getVoiceClientManager()->close();
		break;

	case TYPE_STEP_FORWARD:
	case TYPE_STEP_BACKWARD: {
		FIND_PLAYER_RET(lBusinessID, "step forward/backward")
		player->step2NextFrame(TYPE_STEP_FORWARD == bizParams.BizType ? 1 : 0);
	}
	break;

	case TYPE_STEP_EXIT: {
		FIND_PLAYER_RET(lBusinessID, "step exit")
		player->exitStep();
	}
	break;

	case TYPE_SET_RtspUrl: {
		FIND_PLAYER_RET(lBusinessID, "set url")
		player->setUrl(bizParams);
	}
	break;

	case TYPE_GET_PIC_PARAMS: {
		FIND_PLAYER_RET(lBusinessID, "get picture params")
		auto info = player->getPictureParams();
		if (len > 0 && (info.size() >= len)) {
			AS_LOG(AS_LOG_WARNING, "process %d command fail, buffer is not enough to recevie data.", bizParams.BizType);
		}
		if (!info.empty()) {
			strcpy(pResultData, info.data());
		}
	}
	break;
	
	default:
		break;
	}
	return ret;
}

int32_t CAllplayerManager::excuteInternal(ExcuteInfo& excute, char* pResultData, int len)
{
	int ret = 0;
	AvsPlayer* player = nullptr;

	switch (excute.bizType) {
	case TYPE_RENDER_TYPE: {
		RENDER_TYPE* renderType = (RENDER_TYPE*)excute.varInfo;
		ViewFactory::getInstance()->setRenderType(*renderType);
	}
	break;

	case TYPE_DIGITAL_START:
	case TYPE_DIGITAL_UPDATE: {
#ifndef _WIN32
		ret = AS_ERROR_CODE_INVALID;
		break;
#else
		FIND_PLAYER_RET(excute.bizId, "zoom");
		ZoomSt* zoomSt = (ZoomSt*)excute.varInfo;
		if (!zoomSt->zoomHwnd) {
			AS_LOG(AS_LOG_WARNING, "zoom hwnd is NULL");
			return AS_ERROR_CODE_FAIL;
		}
		player->zoomOut(*zoomSt);
#endif // _WIN32
	}
	break;

	case TYPE_CAPTURE_FRAMES: {
		FIND_PLAYER_RET(excute.bizId, "capture frames")
		CaptureSt* capSt = (CaptureSt*)excute.varInfo;
		player->captureFrames(capSt->capturePath, capSt->captureCount);
	}
	break;

	case TYPE_SKIP_NOKEY_FRAME: {
		FIND_PLAYER_RET(excute.bizId, "skip non key frame")
		uint8_t skip = *(uint8_t*)excute.varInfo;
		player->setSkipNokey(skip);
		//avs_player_factory::GetInstance()->setSkipNokey(skip);
	}
	break;

	case TYPE_PICTURE_PARAMS: {
		FIND_PLAYER_RET(excute.bizId, "adjust picture params")
		std::string* picParams = (string*)excute.varInfo;
		ret = player->adjustPictureParams(*picParams);
	}
	break;

	case TYPE_NETRECORD_CONTROL:
	case TYPE_MULTI_RECORD_CONTROL: {
		FIND_PLAYER_RET(excute.bizId, "control")
		VcrControllSt vcrControll = *(VcrControllSt*)excute.varInfo;
		ret = player->vcrControl(vcrControll.start, vcrControll.scale, vcrControll.scaleOrSpeed);
	}
	break;
	
	case TYPE_URL_SPEED: {
		FIND_PLAYER_RET(excute.bizId, "url speed")
		VcrControllSt vcrControll = *(VcrControllSt*)excute.varInfo;
		player->setScale(vcrControll.scale);
	}
	break;
	
	case TYPE_URL_SEEK: {
		FIND_PLAYER_RET(excute.bizId, "seek")
		SeekSt* seek = (SeekSt*)excute.varInfo;
		player->seek(seek->position);
	}
	break;

	case TYPE_AUDIO_TALK_START:
	case TYPE_VOICE_BROADCASR_START:
	case TYPE_FILE_BROADCASR_START:
	{
		VoiceSt* voice = (VoiceSt*)excute.varInfo;
		auto manager = voice_client_manager::getVoiceClientManager();
		auto talk = manager->getVoiceClient(*voice);
		if (!talk || (ret = talk->init(*voice)) || (ret = talk->start_1())) {
			break;
		}
	}
	break;

	case TYPE_GET_EXPERIENCE_DATA: {
		if (excute.varInfo) {
			ExpCollector* collector = (ExpCollector*)excute.varInfo;
			cJSON* collectors = nullptr;
			for (int i = 0; i < collector->count; ++i) {
				AvsPlayer* player = FindAvsPlayerByBizId(collector->bizArr[i], "", false);
				if (!player) {
					continue;
				}

				if (!collectors && !(collectors = cJSON_CreateArray())) {
					return AS_ERROR_CODE_MEM;
				}

				auto data = player->getExperienceData();
				cJSON_AddItemToArray(collectors, data);
			}

			if (collectors) {
				char* strCollector = cJSON_Print(collectors);
				int strLen = strlen(strCollector);
				if (len > 0 && strLen >= len) {
					AS_LOG(AS_LOG_WARNING, "process %d command exception, buffer is not enough to recevie data.", excute.bizType);
				}
				strcpy(pResultData, strCollector);
				free(strCollector);
				cJSON_Delete(collectors);
			}
		}
	}
	break;

	default:
	/*{
		auto playerMap = avs_player_factory::GetInstance()->getPlayerMap();
		cJSON* collectors = nullptr;
		for (auto& iter : playerMap)
		{
			if (!collectors && !(collectors = cJSON_CreateArray()))
				return AS_ERROR_CODE_MEM;

			auto data = iter.second->get_experience_data();
			cJSON_AddItemToArray(collectors, data);
		}

		if (collectors)
		{
			static char exp[1024] = { 0 };
			memset(exp, 0, 1024);
			char* strCollector = cJSON_Print(collectors);
			strcpy(exp, strCollector);
			AS_LOG(AS_LOG_INFO, "experience upload : %s", exp);
			free(strCollector);
			cJSON_Delete(collectors);
		}
	}*/
		break;
	}
	return ret;
}

void CAllplayerManager::registerStatusCallback(allplay_status_callback pCallBack) {
	avs_player_factory::GetInstance()->registerStatusCallback(pCallBack);
}

void CAllplayerManager::registerDataCallback(allplay_progress_callback pCallBack) {
	avs_player_factory::GetInstance()->registerDataCallback(pCallBack);
}

#if ANDROID
void CAllplayerManager::setNativeWindow(long lBusinessID, ANativeWindow* win)
{
	if (!m_windowsMap.count(lBusinessID)) {
		m_windowsMap.insert(pair<long, ANativeWindow*>(lBusinessID, win));
	} else {
		m_windowsMap[lBusinessID] = win;
	}
}

ANativeWindow* CAllplayerManager::getNativeWindow(long lBusinessID)
{
	if (m_windowsMap.count(lBusinessID)) {
		return m_windowsMap[lBusinessID];
	}
	return nullptr;
}

void CAllplayerManager::changeSurface(long lBusinessID, ANativeWindow* win, int with, int height)
{
	auto player = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
	if (player) {
		m_windowsMap[lBusinessID] = win;
		player->changeSurface(win, with, height);
	}
}

void CAllplayerManager::clearSurface(long lBusinessID)
{
	auto player = avs_player_factory::GetInstance()->findPlayerByBusinessID(lBusinessID);
	if (player) {
		player->clearSurface();
	}
}

void CAllplayerManager::collectAudioFrame(unsigned char* data, int dataSize)
{
	AudioCollector::GetInstance()->pushAudioFrame(data, dataSize);
}

void CAllplayerManager::stopAudioRecord()
{
	AudioCollector::GetInstance()->stopRecord();
}
#endif
