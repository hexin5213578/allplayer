#include "crypto_muxer.h"

using namespace crypto;

CryptoMuxer::CryptoMuxer(long id, int32_t type, FileDumpSt& dump):
	avs_egress(id, type),
	m_dumpSt(dump), m_writer(nullptr){
}

CryptoMuxer::~CryptoMuxer()
{
	stop();
}

int CryptoMuxer::init(std::string& output, MK_Format_Contex* format)
{
	m_writer = new CryptoWriter(output, m_dumpSt.key, m_dumpSt.limit_days, m_dumpSt.limit_times);
	if (!m_writer) {
		return AVERROR(ENOMEM);
	}
	m_format = format;
	if (!m_writer->open()) {
		return AVERROR(EINVAL);
	}
	return m_writer->generateHeader(format);
}

int CryptoMuxer::do_egress(AVPacket* pkt)
{
	if (!m_writer) {
		return AVERROR(EINVAL);
	}
		
	if (m_format->video_stream == pkt->stream_index) {
		if (pkt->flags & AV_PKT_FLAG_KEY) {
			m_writer->writeFrameData(0, pkt, true) ? 0 : AVERROR(EINVAL);
		}
		else {
			m_writer->writeFrameData(0, pkt, false) ? 0 : AVERROR(EINVAL);
		}
	}
	else if(m_format->audio_stream == pkt->stream_index) {
		m_writer->writeFrameData(1, pkt, false) ? 0 : AVERROR(EINVAL);
	}
	else {
	
	}
    return 0;
}

int CryptoMuxer::write_trailer()
{
	if (!m_writer) {
		return AVERROR(EINVAL);
	}
	int ret = m_writer->writeTail() ? 0 : AVERROR(EINVAL);
	delete m_writer;
	m_writer = nullptr;
	return ret;
}

int CryptoMuxer::stop()
{
	int ret = 0;
	if (m_writer) {
		ret = write_trailer();
	}
	return ret;
}








