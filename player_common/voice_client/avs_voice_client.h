#pragma once
#include "avs_player_common.h"
#include "avs_player_factory.h"

class avs_voice_client {
public:
	enum AudioCientState {
		kIdle = 0,
		kConnected = 1,
		kPushing = 2,
	};

	avs_voice_client() :
		m_state(kIdle),
		m_voiceParams(-1, -1) {
	}

	virtual int init(VoiceSt& params) = 0;
	virtual int start_1() = 0;
	virtual void stop_1() = 0;

protected:
	void sendStatus(long status, std::string description) {
		avs_player_factory::GetInstance()->sendStatusMsg(m_voiceParams.bizId, m_voiceParams.bizType,
			status, description);
	}

	AudioCientState		m_state;
	VoiceSt				m_voiceParams;
};

class voice_client_manager {
public:
	static voice_client_manager* getVoiceClientManager() {
		static voice_client_manager manager;
		return &manager;
	}

	avs_voice_client* getVoiceClient(VoiceSt& voice);

	void close() {
		if (m_client) {
			m_client->stop_1();
		}
		m_client = nullptr;
	}

protected:
	voice_client_manager() = default;
	~voice_client_manager() = default;

protected:
	avs_voice_client* m_client = nullptr;
};