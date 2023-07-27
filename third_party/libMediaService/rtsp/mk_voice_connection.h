#pragma once
#include "mk_rtsp_connection.h"
#include "wav_handle.h"

enum eRtpPayloadType
{
    RTP_PAYLOAD_TYPE_PCMU = 0,   // g711u
    RTP_PAYLOAD_TYPE_PCMA = 8,   // g711a
    RTP_PAYLOAD_TYPE_JPEG = 26,
    RTP_PAYLOAD_TYPE_H264 = 96,
    RTP_PAYLOAD_TYPE_H265 = 97,
    RTP_PAYLOAD_TYPE_OPUS = 98,
    RTP_PAYLOAD_TYPE_AAC = 99,
    RTP_PAYLOAD_TYPE_G726 = 100,
    RTP_PAYLOAD_TYPE_G726_16 = 101,
    RTP_PAYLOAD_TYPE_G726_24 = 102,
    RTP_PAYLOAD_TYPE_G726_32 = 103,
    RTP_PAYLOAD_TYPE_G726_40 = 104,
    RTP_PAYLOAD_TYPE_SPEEX = 105,
};

enum voiceErrorCode
{
    kConnectError = 101,            //tcp连接失败
    kSetupTimeout = 102,            //rtsp建连超时
    kSetupError = 103,              //rtsp连接建立失败
    kNotSupportSdp = 104,           //不支持的音频sdp(只支持g711)
    kColleterError = 105,           //麦克风采集失败
    kServerError = 106,             //服务端返回异常
    kChannelBusy = 107,             //音频通道被占用
    kConnClosedByPeer = 108,        //连接被对端关闭
    kOpenWavFileError = 109,        //打开wav音频文件失败
    kNotSupportWav = 110,           //不支持的wav文件格式
    kPackedRtpError = 201,          //rtp打包发送失败
    kErrorMax = 9999,                
};

class voice_rtsp_connection : public mk_rtsp_connection
{
public:
	voice_rtsp_connection();
	virtual ~voice_rtsp_connection();

    int32_t start() override;

    virtual void stop() override;

    int32_t set_wav_config(const char* wav, char loop);

public:
	virtual void handle_send(void) override;

protected:
	virtual void check_client()  override;

    int32_t sendRtspSetupReq(SDP_MEDIA_INFO* info) override;

    virtual int32_t handleRtspDescribeResp(mk_rtsp_packet& rtspMessage) override;

    virtual int32_t handleRtspSetUpResp(mk_rtsp_packet& rtspMessage) override;

private:
    int pack_rtp_packet(char* data, uint32_t len, uint8_t channel);

    void statusCallback(MR_CLIENT_STATUS status, voiceErrorCode code);

private:
    unsigned char*      m_pAudioData;
    int                 m_iStreamIdx;

    uint16_t            m_seqNumber;
    uint32_t            m_timestamp;
    uint32_t            m_ssrcVal;
    CWavHandle          m_wavHandle;
    uint32_t            m_wavRemain;
    const char*         m_wavFile;
    char                m_wavLoop;
    int                 m_sampleRate;
    int                 m_channels;
    int                 m_bytesPerSample;
    uint32_t            m_ulSentMs;
};
