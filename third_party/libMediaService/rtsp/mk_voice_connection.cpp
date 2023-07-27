#include "mk_voice_connection.h"
#include "mk_media_common.h"
#include "mk_rtsp_rtp_packet.h"
#include "as_config.h"

#include "avs_player_factory.h"

#if (AS_APP_OS & AS_OS_WIN) == AS_OS_WIN
#include "pusher/g711_encode.h"
#include "pusher/audio_collect.h"
#include "extend/as_frame.h"
#elif (AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID
#include "pusher/g711_encode.h"
#include "pusher/audio_collector.h"
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_IOS) == AS_OS_IOS)
#include "ios_collector.h"
#include "g711_encode.h"
#include "as_frame.h"

#endif

#include <cstring>
#include <sstream>

static FILE* pf = nullptr;

#define VOICE_SETUP_TIMEOUT 70 

#define G711ENCODE  //是否需要g711编码
#define AUDIO_DATA_SIZE  (2 * 1024)

voice_rtsp_connection::voice_rtsp_connection()
    :m_iStreamIdx(-1),
    m_pAudioData(nullptr),
    m_seqNumber(0), m_timestamp(0),
    m_wavFile(nullptr), m_wavRemain(0),
    m_ulSentMs(0)
{
    m_bVoiceTalk = true;
}

voice_rtsp_connection::~voice_rtsp_connection()
{
    stop();
}

int32_t voice_rtsp_connection::start()
{
    if (!m_wavFile) {
#if (AS_APP_OS & AS_OS_WIN) == AS_OS_WIN
        AudioCollect::GetInstance().stopIngress();

        long result = 0;
        if (result = AudioCollect::GetInstance().Init())
        {
            MK_LOG(AS_LOG_WARNING, "voice talk init colllector fail, %ld.", result);
            statusCallback(MR_CLIENT_VOICE_START_FAIL, kColleterError);
            return -1;
        }

        result = AudioCollect::GetInstance().startIngress();
        if (VALUE_OK != result)
        {
            MK_LOG(AS_LOG_ERROR, "voice talk colllector ingress fail, %ld.", result);
            statusCallback(MR_CLIENT_VOICE_START_FAIL, kColleterError);
            return  -1;
        }
#endif
    }

    auto ret = setupConnection();
    if (AS_ERROR_CODE_OK != ret) 
    {
        statusCallback(MR_CLIENT_VOICE_START_FAIL, kConnectError);
        return ret;
    }

    if (!(m_pAudioData = AS_NEW(m_pAudioData, AUDIO_DATA_SIZE)))
    {
        statusCallback(MR_CLIENT_VOICE_START_FAIL, kColleterError);
        return AS_ERROR_CODE_MEM;
    }
    
    if (!m_bSocks)
    {
        //setHandleSend(AS_TRUE);
        if ((ret = this->sendRtspDescribeReq()))
        {
            statusCallback(MR_CLIENT_VOICE_START_FAIL, kSetupError);
            return ret;
        }
        setHandleRecv(AS_TRUE);
    }
    return ret;
}

void voice_rtsp_connection::stop()
{
    if (!m_wavFile)
    {
#if (AS_APP_OS & AS_OS_WIN) == AS_OS_WIN
        AudioCollect::GetInstance().stopIngress();
#elif (AS_APP_OS & AS_OS_IOS) == AS_OS_IOS
        IOS_AudioCollector::GetInstance()->stopRecord();
#endif
    }
    mk_rtsp_connection::stop();
    AS_DELETE(m_pAudioData);
    m_wavHandle.CloseFlie();
}

int32_t voice_rtsp_connection::set_wav_config(const char* wav, char loop)
{
    if (!wav) 
        return AS_ERROR_CODE_FAIL;
    
    int ret = 0;
    if (ret = m_wavHandle.OpenWavFile(wav)) {   
        statusCallback(MR_CLIENT_VOICE_START_FAIL, kOpenWavFileError);
        return ret;
    }
      
    m_sampleRate = m_wavHandle.GetSampleRate();
    m_channels = m_wavHandle.GetChannels();
    m_bytesPerSample = m_wavHandle.GetBitsPerSample() / 8;
    m_wavRemain = m_wavHandle.GetDataLength();

    static const int kSampleRate = 8000;
    if (kSampleRate != m_sampleRate)
    {
        MK_LOG(AS_LOG_WARNING, "%d sample rate wav file not support, please convrt to 8k sample rate.", m_sampleRate);
        statusCallback(MR_CLIENT_VOICE_START_FAIL, kNotSupportWav);
        return AS_ERROR_CODE_UNSUPPORT;
    }
    m_wavFile = wav;
    m_wavLoop = loop;
    return ret;
}

void voice_rtsp_connection::check_client()
{
    time_t cur = time(NULL);

    if (!m_bSetupSuccess) {
        if (VOICE_SETUP_TIMEOUT < (cur - m_ulRtcpStartTime)) {
            handle_connection_status(MR_CLIENT_SETUP_TIMEOUT);
            m_ulRtcpStartTime = cur;
        }
    }
    return;
}

int32_t voice_rtsp_connection::sendRtspSetupReq(SDP_MEDIA_INFO* info)
{
    std::string strTransport = "";

    // (RTP)
    strTransport += RTSP_TRANSPORT_RTP;
    strTransport += RTSP_TRANSPORT_SPEC_SPLITER;
    strTransport += RTSP_TRANSPORT_PROFILE_AVP;

    strTransport += RTSP_TRANSPORT_SPEC_SPLITER;
    strTransport += RTSP_TRANSPORT_TCP;
    strTransport += SIGN_SEMICOLON;

    strTransport += RTSP_TRANSPORT_UNICAST;
    strTransport += SIGN_SEMICOLON;
    strTransport += RTSP_TRANSPORT_INTERLEAVED;
    std::stringstream strChannelNo;
    strChannelNo << 0;
    strTransport += strChannelNo.str() + SIGN_H_LINE;
    strChannelNo.str("");
    strChannelNo << 1;
    strTransport += strChannelNo.str();
    
    std::string strUri = STR_NULL;
	if ("" != info->strControl && !strncmp(info->strControl.c_str(), "trackID=", strlen("trackID="))) {
		std::string Url = std::string("rtsp://") + (char*)&m_url.host[0];
		if (0 != m_url.port) {
			std::stringstream strPort;
			strPort << m_url.port;
			Url += ":" + strPort.str();
		}
		Url += (char*)&m_url.uri[0];
		Url += "/";
		Url += info->strControl;
		strUri = Url;
	} else if ("" != info->strControl) {
		strUri = info->strControl;
	}
    
    return sendRtspReq(RtspSetupMethod, strUri, strTransport, m_sessionId);
}

int32_t voice_rtsp_connection::handleRtspDescribeResp(mk_rtsp_packet& rtspMessage)
{
	int32_t ret = mk_rtsp_connection::handleRtspDescribeResp(rtspMessage);
    int alawIdx = -1;

    if (m_pFormatCtx) 
    {
        for (int idx = 0; idx < m_pFormatCtx->streams.size(); ++idx) 
        {
            auto stream = m_pFormatCtx->streams[idx];
            auto codecPara = stream->codecpar;
            if (MEDIA_TYPE_AUDIO == codecPara->codec_type)
            {
                if (MK_CODEC_ID_PCM_ALAW == codecPara->codec_id) 
                {
                    alawIdx = idx;
                    break;
                }
                else if(MK_CODEC_ID_PCM_MULAW == codecPara->codec_id)
                {
                    alawIdx = idx;
                    break;
                }
                else {          // not support
                }
            }
        }
    }

    if (alawIdx >= 0) {
        m_iStreamIdx = alawIdx;
    }
   
    if(m_iStreamIdx < 0){
        ret = AS_ERROR_CODE_INVALID;
        statusCallback(MR_CLIENT_VOICE_START_FAIL, kNotSupportSdp);
    }
    return ret;
}

int32_t voice_rtsp_connection::handleRtspSetUpResp(mk_rtsp_packet& rtspMessage)
{
    MK_LOG(AS_LOG_INFO, "rtsp client connection handle setup response begin.");
    int32_t nRet = AS_ERROR_CODE_OK;
    if (0 < m_mediaInfoList.size()) {
        MK_LOG(AS_LOG_INFO, "voice client connection handle setup response,send next setup messgae.");
        SDP_MEDIA_INFO* info = m_mediaInfoList.front();
        if(AS_ERROR_CODE_OK != (nRet = sendRtspSetupReq(info))) {
            statusCallback(MR_CLIENT_VOICE_START_FAIL, kSetupError);
            return AS_ERROR_CODE_FAIL;
        }
        m_mediaInfoList.pop_front();
    }
    else {
        m_Status = RTSP_SESSION_STATUS_SETUP;
        handle_connection_status(MR_CLIENT_STATUS_HANDSHAKE);
        if (AS_ERROR_CODE_OK != (nRet = sendRtspRecordReq())) {
            statusCallback(MR_CLIENT_VOICE_START_FAIL, kSetupError);
            return AS_ERROR_CODE_FAIL;
        }

        if(m_iStreamIdx >=0) {
            if (!m_wavFile) {
#if (AS_APP_OS & AS_OS_WIN) == AS_OS_WIN
              
#elif (AS_APP_OS & AS_OS_IOS) == AS_OS_IOS
                IOS_AudioCollector::GetInstance()->startRecord();
#endif
            }
            time_t tm = time(NULL);//得到系统时间
            srand((unsigned int) tm);//随机种子只需要设置一次即可
            m_ssrcVal = rand();
            m_bSetupSuccess = true;
            handle_connection_status(MR_CLIENT_VOICE_START_SUCCESS);
        }
    }

    /*#ifdef	G711ENCODE
        pf = fopen("D:\\test.g711", "wb");  
    #else
        pf = fopen("D:\\test.pcm", "wb");
    #endif*/
    MK_LOG(AS_LOG_INFO, "voice connection handle setup response success.");
    return nRet;
}

int voice_rtsp_connection::pack_rtp_packet(char* data, uint32_t len, uint8_t channel)
{
    if (len >= AUDIO_DATA_SIZE) {
        AS_LOG(AS_LOG_WARNING, "rtp pack size %d is larger than AUDIO_DATA_SIZE.", len);
        return AS_ERROR_CODE_MEM;
    }
 
    int ret = 0;
    mk_rtp_packet packet;
    char packet_data[AUDIO_DATA_SIZE] = { 0 };
    memcpy(packet_data + RTP_INTERLEAVE_LENGTH + sizeof(RTP_FIXED_HEADER), data, len);
    packet.GeneratePacket(packet_data, len, 0);
    packet.SetSeqNum(m_seqNumber++);
    packet.SetTimeStamp(m_timestamp);
    m_timestamp += len;
    packet.SetPayloadType(m_pFormatCtx->streams[m_iStreamIdx]->payload);
    packet.SetSSRC(m_ssrcVal);
    ret = sendMsg((const char*)packet_data, RTP_INTERLEAVE_LENGTH + sizeof(RTP_FIXED_HEADER) + len);
    m_ulSentMs = as_get_cur_msecond();
    return ret;
}

void voice_rtsp_connection::statusCallback(MR_CLIENT_STATUS status, voiceErrorCode code)
{
    MEDIA_STATUS_INFO st{ status, code };
    handle_connection_status(st);
}

int StereoToMono(unsigned char* dstCode, unsigned char* srcCode, int nBufferSize)
{
    if (dstCode == NULL || srcCode == NULL || nBufferSize <= 0) {
        return 0;
    }

    for (int i = 0, j = 0; i < nBufferSize; i += 4, j += 2) {
        dstCode[j] = (unsigned char)(srcCode[i]);
        dstCode[j + 1] = (unsigned char)(srcCode[i + 1]);
    }
    return nBufferSize / 2;
}

void voice_rtsp_connection::handle_send(void)
{
    if (!m_bSetupSuccess || m_iStreamIdx < 0 || !m_pAudioData) {
        setHandleSend(AS_TRUE);
        return;
    }

	setHandleSend(AS_FALSE);
    unsigned int dataLen = 0;
    if (!m_wavFile) 
	{
#if (AS_APP_OS & AS_OS_WIN) == AS_OS_WIN
        AudioCollect::GetInstance().GetAudioData(m_pAudioData, &dataLen);
#elif (AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID
        AudioCollector::GetInstance()->popAudioFrame(m_pAudioData, &dataLen);
#elif (AS_APP_OS & AS_OS_IOS) == AS_OS_IOS
        IOS_AudioCollector::GetInstance()->getAudioFrame(m_pAudioData, &dataLen);
#endif
    }
    else 
	{
		static const int fps = 25;
        static const int miliPerSecond = 1000;
		if (as_get_cur_msecond() - m_ulSentMs < miliPerSecond / fps) {
			setHandleSend(AS_TRUE);
			return;
		}
		
        dataLen = m_sampleRate * m_bytesPerSample * m_channels / fps;
        if (m_wavRemain < dataLen)  {
            dataLen = m_wavRemain;
        }

        if (0 == dataLen && !m_wavLoop) {
            setHandleSend(AS_TRUE);
            return;
        }

        dataLen = m_wavHandle.ReadData(m_pAudioData, dataLen);
        m_wavRemain -= dataLen;
        if ((0 == m_wavRemain) || (dataLen == 0)) {
            if (m_wavLoop) {
                m_wavHandle.SetPosition(0);
                m_wavRemain = m_wavHandle.GetDataLength();
            }
            else {
                handle_connection_status(MR_CLIENT_VOICE_EOS);
            }
        }

        if (2 == m_channels) {        //convert to single channel
            dataLen = StereoToMono(m_pAudioData, m_pAudioData, dataLen);
        }
    }

    if (dataLen) 
    {
        switch (m_pFormatCtx->streams[m_iStreamIdx]->codecpar->codec_id)
        {
        case MK_CODEC_ID_PCM_ALAW:
            dataLen = G711EnCode(m_pAudioData, (char*)m_pAudioData, dataLen ,0);
            break;
        case MK_CODEC_ID_PCM_MULAW:
            dataLen = G711EnCode(m_pAudioData, (char*)m_pAudioData, dataLen, 1);
            break;
        default:
            setHandleSend(AS_TRUE);
            return;
        }

        if (pf) {
            fwrite(m_pAudioData, dataLen, 1, pf);
        }
        
        int ret = pack_rtp_packet((char*)m_pAudioData, dataLen, 0);
        if (ret && AS_ERROR_CODE_MEM != ret)
        {
            statusCallback(MR_CLIENT_VOICE_FAIL, kPackedRtpError);
        }
    }
	setHandleSend(AS_TRUE);
}
