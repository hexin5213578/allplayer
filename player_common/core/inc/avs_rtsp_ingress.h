#ifndef __AVS_INGRESS_STREAM_H__
#define __AVS_INGRESS_STREAM_H__

#include "avs_ingress.h"

class AvsRtspIngress: public avs_ingress
{
public:
    AvsRtspIngress();
	virtual ~AvsRtspIngress() { stop_ingress(); };

    int32_t start_ingress(int16_t fragments, bool voice) override;
    void stop_ingress() override;
	void pause_ingress() override;
	
	void set_vcr_initial_parameter(const VcrControllSt& vcst, uint8_t highSpeedCache, int32_t bizType);

	void set_scale_speed(uint8_t highSpeedCache) {
		m_initial_vcrSt.scaleOrSpeed = highSpeedCache;
	}

	int32_t vcr_control(double dropPos, double scale) override;
	
	int32_t get_stream_stat(RTP_PACKET_STAT_INFO& stat) override;
	MK_Format_Contex* get_format_context() override;
	
	void set_wav_config(std::string wav, char loop);

	void set_stream_speed(double& scale);

public:
	int32_t av_client_handle_status(MR_CLIENT client, MEDIA_STATUS_INFO status, void* ctx);
	int32_t av_lib_media_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len, void* ctx);
	char*   av_handle_buffer(MR_CLIENT client, uint32_t len, uint32_t& ulBufLen, void* ctx);

public:
	static int32_t rtxp_client_handle_status(MR_CLIENT client, MEDIA_STATUS_INFO status, void* ctx)  {
		AvsRtspIngress* streamIngress = (AvsRtspIngress*)ctx;
		return streamIngress->av_client_handle_status(client, status, ctx);
	}

	static int32_t rtxp_client_handle_media(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len, void* ctx) {
		AvsRtspIngress* streamIngress = (AvsRtspIngress*)ctx;
		return streamIngress->av_lib_media_data(client, dataInfo, len, ctx);
	}

	static char* rtxp_client_handle_buffer(MR_CLIENT client, uint32_t len, uint32_t& ulBufLen, void* ctx) {
		AvsRtspIngress* streamIngress = (AvsRtspIngress*)ctx;
		return streamIngress->av_handle_buffer(client, len, ulBufLen, ctx);
	}

private:
	MEDIA_CALL_BACK   m_media_cb;
	MR_CLIENT         m_handle;
	std::string		  m_wav_file;
	char			  m_loop;
	uint8_t			  m_highSpeedCache;
	int				  m_bizType;
	VcrControllSt	  m_initial_vcrSt;
};

#endif /* __AVS_INGRESS_STREAM_H__ */