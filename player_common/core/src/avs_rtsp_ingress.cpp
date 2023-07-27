#include "avs_rtsp_ingress.h"
#include "as_common.h"
#include "socks_config.h"

AvsRtspIngress::AvsRtspIngress() {
	m_handle = nullptr;
	m_media_cb.ctx = this;
	m_highSpeedCache = 0;
	//m_initial_vcrSt = {0.0, 1.0 , 0};
	m_media_cb.m_cb_status = rtxp_client_handle_status;
	m_media_cb.m_cb_data = rtxp_client_handle_media;
	m_media_cb.m_cb_buffer = rtxp_client_handle_buffer;
}

int32_t AvsRtspIngress::start_ingress(int16_t fragments, bool voice)
{
	m_handle = mk_create_client_handle((char*)m_url.data(), &m_media_cb, this, voice);
	if (!m_handle) {
		AS_LOG(AS_LOG_ERROR, "create client handle failed");
		return AS_ERROR_CODE_FAIL;
	}
	//内外网环境下udp，需要NAT，故默认tcp
	mk_set_rtsp_client_over_tcp(m_handle);
	//mk_set_client_fragment_count(m_handle, fragments);
	mk_set_client_send_rtcp(m_handle, true);

	mk_set_vcr_parameter(m_handle, m_initial_vcrSt);
	int ret = AS_ERROR_CODE_OK;
	if (voice && !m_wav_file.empty()) {
		if (ret = mk_set_wav_conf(m_handle, m_wav_file.c_str(), m_loop)) {
			return ret;
		}
	}
	
	SocksInfo* socks_info = SocksConfig::GetSocksConfigInstance()->getSocksInfo();
	if (socks_info) {
		mk_client_set_socks(m_handle, socks_info->ip.c_str(), socks_info->port,
			socks_info->username.c_str(), socks_info->password.c_str());
	}
	
	ret = mk_start_client_handle(m_handle);
    return ret;
}

void AvsRtspIngress::stop_ingress()
{
	if (!m_handle) {
		return;
	}
	mk_stop_client_handle(m_handle);
	mk_destory_client_handle(m_handle);
	m_handle = nullptr;
    return;
}

void   AvsRtspIngress::pause_ingress()
{
	if (NULL == m_handle) {
		return;
	}
	mk_create_rtsp_client_pause(m_handle);
	return;
}

void AvsRtspIngress::set_vcr_initial_parameter(const VcrControllSt& vcst, uint8_t highSpeedCache, int32_t bizType)
{
	m_initial_vcrSt = vcst;
	m_highSpeedCache = highSpeedCache;
	m_bizType = bizType;
	set_stream_speed(m_initial_vcrSt.scale);
}

int32_t AvsRtspIngress::vcr_control(double dropPos, double scale)
{
	if (NULL == m_handle) {
		return AS_ERROR_CODE_FAIL;
	}

	set_stream_speed(scale);

	if (kScale == (SpeedEnum)m_initial_vcrSt.scaleOrSpeed) {
		mk_create_rtsp_play_control(m_handle, dropPos, scale, 0.0);
	}
	else if(kSpeed == (SpeedEnum)m_initial_vcrSt.scaleOrSpeed){
		mk_create_rtsp_play_control(m_handle, dropPos, 0.0, scale);
	}
	return 0;
}

int32_t AvsRtspIngress::get_stream_stat(RTP_PACKET_STAT_INFO& stat)
{
	if (NULL == m_handle) {
		AS_LOG(AS_LOG_WARNING, "get stream stat fail, not client handle.");
		return -1;
	}
	
	mk_get_client_rtp_stat_info(m_handle,&stat);
	AS_LOG(AS_LOG_INFO, "ulLostRtpPacketNum : %d, ulTotalPackNum : %d, lostRate: %lf",
		stat.ulLostRtpPacketNum, stat.ulTotalPackNum,
		stat.ulTotalPackNum ? (double)stat.ulLostRtpPacketNum / (double)stat.ulTotalPackNum : 0.0);
	
	return 0;
}

MK_Format_Contex* AvsRtspIngress::get_format_context()
{
	return mk_get_client_av_format(m_handle);
}

void AvsRtspIngress::set_wav_config(std::string wav, char loop)
{
	m_wav_file = wav;
	m_loop = loop;
}

void AvsRtspIngress::set_stream_speed(double& scale)
{
	return;
	if (scale > 0 && scale < 32 && m_bizType == TYPE_NETRECORD_START) {
		if (scale != 4.0 && m_highSpeedCache == 1) {
			scale *= 2.0;
		} else if (scale < 1.0) {
			scale = 1.0;
		}
	}
}

int32_t AvsRtspIngress::av_client_handle_status(MR_CLIENT client, MEDIA_STATUS_INFO status, void* ctx) {
	if (!m_ingreeData) {
		return -1;
	}
	return m_ingreeData->handle_ingress_status(status);
}

int32_t AvsRtspIngress::av_lib_media_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len, void* ctx) {
	if (!m_ingreeData) {
		return -1;
	}
	return m_ingreeData->handle_ingress_data(client, dataInfo, len);
}

char* AvsRtspIngress::av_handle_buffer(MR_CLIENT client, uint32_t len, uint32_t& ulBufLen, void* ctx) {
	if (!m_ingreeData) {
		return nullptr;
	}
	return m_ingreeData->alloc_ingress_data_buf(len, ulBufLen);
}