#ifndef CRYPTO_MUXER_H__
#define CRYPTO_MUXER_H__

#include "avs_egress.h"
#include "crypto_writer.h"

class CryptoMuxer : public avs_egress
{
public:
	CryptoMuxer(long id, int32_t type, FileDumpSt& dump);
	virtual ~CryptoMuxer();

	int init(std::string& output, MK_Format_Contex* format) override;
	int do_egress(AVPacket* pkt) override;
	
	uint64_t getFileSize() override { return m_writer ? m_writer->getFileSize() : 0; }
	
	int64_t getDuration() override { return  m_writer ? m_writer->getDuration() : 0; }
	
	bool remuxing() override { return !!m_writer; }
	
	int write_trailer() override;
	
	int stop() override;

private:
	FileDumpSt				m_dumpSt;
	crypto::CryptoWriter*	m_writer;
};

#endif	//