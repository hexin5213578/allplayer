#include "avs_rtsp_voice_client.h"
#include "avs_rtsp_ingress.h"
#include "mk_voice_connection.h"
#include "as_mem.h"

rtsp_voice_client* rtsp_voice_client::get_audio_talk_instance() {
	static rtsp_voice_client client;
	return &client;
}

rtsp_voice_client* rtsp_voice_client::get_voice_broadcast_instance() {
	static rtsp_voice_client voice_broadcast;
	return &voice_broadcast;
}

rtsp_voice_client* rtsp_voice_client::get_file_broadcast_instance() {
	static rtsp_voice_client file_broadcast;
	return &file_broadcast;
}

rtsp_voice_client::rtsp_voice_client()
	:avs_voice_client(), 
	m_streamIngress(nullptr){
}

rtsp_voice_client::~rtsp_voice_client() {
	if (AudioCientState::kIdle < m_state) {
		stop_1();
	}
	AS_DELETE(m_streamIngress);
}

int rtsp_voice_client::init(VoiceSt& params) {
	stop_1();

	m_voiceParams = params;
	if (!m_streamIngress) {
		m_streamIngress = AS_NEW_REAL<avs_ingress, AvsRtspIngress>(m_streamIngress);
		if (!m_streamIngress) {
			return AS_ERROR_CODE_MEM;
		}
	}
	
	int ret = m_streamIngress->init(params.voiceUrl, this);
	if (AS_ERROR_CODE_OK != ret) {
		AS_LOG(AS_LOG_ERROR, "avs_ingress_stream init failed !");
	}
	return ret;
}

int rtsp_voice_client::start_1() {
	int ret = 0;
	if (m_streamIngress) {
		//if (TYPE_FILE_BROADCASR_START == m_bizParams.BizType) {
		if(!m_voiceParams.wavFile.empty()) {
			dynamic_cast<AvsRtspIngress*>(m_streamIngress)->set_wav_config(m_voiceParams.wavFile, m_voiceParams.loop);
		}

		if (!(ret = m_streamIngress->start_ingress(1, true))) {
			m_state = AudioCientState::kConnected;
		}
		return ret;
	}
	return AS_ERROR_CODE_NOT_INIT;
}

void rtsp_voice_client::stop_1()
{
	if (m_streamIngress) {
		m_streamIngress->stop_ingress();
		AS_DELETE(m_streamIngress);
	}
	m_state = AudioCientState::kIdle;
}

int32_t rtsp_voice_client::handle_ingress_status(MEDIA_STATUS_INFO ulStatus)
{
	STREAM_STATUS_TYPE status = STREAM_STATUS_NONE;
	std::string errCode;
	switch (ulStatus.enStatus) {
	case MR_CLIENT_VOICE_START_FAIL:
		status = STREAM_VOICE_START_FAIL;
		errCode = std::to_string(ulStatus.errCode);
		break;
	case MR_CLIENT_SETUP_TIMEOUT:
		ulStatus.errCode = kSetupTimeout;
		errCode = std::to_string(ulStatus.errCode);
		status = STREAM_VOICE_START_FAIL;
		break;
	case MR_CLIENT_STATUS_SRV_ERROR: {
		if (AudioCientState::kIdle == m_state) {
			AS_LOG(AS_LOG_WARNING, "voice talk server error when idle.");
			return -1;
		}
		else if (AudioCientState::kConnected == m_state) {
			if (412 == ulStatus.errCode)
				ulStatus.errCode = kChannelBusy;
			else
				ulStatus.errCode = kServerError;

			errCode = std::to_string(ulStatus.errCode);
			status = STREAM_VOICE_START_FAIL;
		}
		else if (AudioCientState::kPushing == m_state) {
			ulStatus.errCode = kServerError;
			errCode = std::to_string(ulStatus.errCode);
			status = STREAM_VOICE_FAIL;
		}
		break;
	}
	case MR_CLIENT_VOICE_START_SUCCESS: {
		status = STREAM_VOICE_START_SUCCESS;
		m_state = AudioCientState::kPushing;
	}
		break;
	case MR_CLIENT_STATUS_CONN_CLOSE: {
		if (AudioCientState::kIdle == m_state) {
			AS_LOG(AS_LOG_WARNING, "voice talk connection close when idle.");
			return -1;
		}
		else if (AudioCientState::kConnected == m_state) {
			ulStatus.errCode = kConnClosedByPeer;
			errCode = std::to_string(ulStatus.errCode);
			status = STREAM_VOICE_START_FAIL;
		}
		else if (AudioCientState::kPushing == m_state) {
			ulStatus.errCode = kConnClosedByPeer;
			errCode = std::to_string(ulStatus.errCode);
			status = STREAM_VOICE_FAIL;
		}
		break;
	}
	case MR_CLIENT_VOICE_FAIL: {
		errCode = std::to_string(ulStatus.errCode);
		status = STREAM_VOICE_FAIL;
		break;
	}
	case MR_CLIENT_VOICE_EOS:
		status = STREAM_VOICE_EOS;
		break;
	default:
		return AS_ERROR_CODE_FAIL;
	}

	if (STREAM_STATUS_NONE != status) {
		AS_LOG(AS_LOG_WARNING, "voice talk status: %d, errCode: %s.", status, errCode.c_str());
		sendStatus((long)status, errCode);
	}
	return AS_ERROR_CODE_OK;
}