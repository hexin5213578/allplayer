#pragma once 
#include "avs_voice_client.h"
#include "avs_ingress.h"

class rtsp_voice_client : public avs_voice_client, public avs_ingress_data {
public:
	static rtsp_voice_client* get_audio_talk_instance();

	static rtsp_voice_client* get_voice_broadcast_instance();

	static rtsp_voice_client* get_file_broadcast_instance();

	int init(VoiceSt& params) override;

	int start_1() override;

	void stop_1() override;

private:
	rtsp_voice_client();
	virtual ~rtsp_voice_client();

protected:
	int32_t handle_ingress_status(MEDIA_STATUS_INFO ulStatus) override;

	/**
	 * 分配空间接收rtsp返回的数据
	 * \param len
	 * \param ulBufLen
	 */
	char* alloc_ingress_data_buf(uint32_t len, uint32_t& ulBufLen) override {
		return nullptr;
	}

	/**
	 * 处理h264，g711数据流
	 * \param client
	 * \param dataInfo
	 * \param len
	 */
	int32_t handle_ingress_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len) override {
		return 0;
	}

private:
	avs_ingress*		m_streamIngress = nullptr;
};
