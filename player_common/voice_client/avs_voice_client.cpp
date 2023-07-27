#include "avs_rtsp_voice_client.h"
#include "avs_rtmp_voice_client.h"
#include "util.h"

avs_voice_client* voice_client_manager::getVoiceClient(VoiceSt& voice) {
	if (m_client) {
		AS_LOG(AS_LOG_WARNING, "there is a voice client here, pelese check and close.");
		return nullptr;
	}

	switch (voice.bizType)
	{
	case TYPE_AUDIO_TALK_START:
		if (start_with(voice.voiceUrl, "rtsp")) {
			m_client = rtsp_voice_client::get_audio_talk_instance();
		}
		else if (start_with(voice.voiceUrl, "rtmp")) {
#ifdef _WIN32
            //TODO iOS音频采集
			m_client = rtmp_voice_client::get_audio_talk_instance();
#endif
		}
		break;

	case TYPE_VOICE_BROADCASR_START:
		if (start_with(voice.voiceUrl, "rtsp")) {
			m_client = rtsp_voice_client::get_voice_broadcast_instance();
		}
		else if (start_with(voice.voiceUrl, "rtmp")) {
			m_client;
		}
		break;

	case TYPE_FILE_BROADCASR_START:
		if (start_with(voice.voiceUrl, "rtsp")) {
			m_client = rtsp_voice_client::get_file_broadcast_instance();
		}
		break;

	default:
		break;
	}

	if (!m_client) {
		AS_LOG(AS_LOG_WARNING, "there is no voice client match.");
	}

	return m_client;
}
