#ifndef CRYPTO_DEMUXER_H__
#define CRYPTO_DEMUXER_H__

#include "avs_thread_base.h"
#include "avs_player.h"
#include "crypto_reader.h"
#include "avs_refresh_loop.h"
#include "avs_parser.h"

#define RECV_DATA_BUF_SIZE (4*1024*1024) 

class CryptoPlayer : public AvsThreadBase, public AvsPlayer, public avs_ingress_data
{
public:
	CryptoPlayer(BizParams& params);
	virtual ~CryptoPlayer();

	int play() override;
	void close();
	void pause(bool pause, bool clear = false) override;
	void seek(double pos) override;
	std::string getPlayInfo() override;

	int32_t handle_ingress_status(MEDIA_STATUS_INFO status) override;
	char* alloc_ingress_data_buf(uint32_t len, uint32_t& ulBufLen) override;
	int32_t handle_ingress_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len) override;

protected:
	void mainProcess() override;
	int doTask(AVPacket* pkt) override;
	int initOutput();
	int readUntil(int64_t target);

private:
	FileDumpSt					m_dumpSt;
	crypto::CryptoReader*		m_reader = nullptr;

	std::string					m_cryptoFile;
	char*						m_mediaBuf;
	double						m_seekFrac;

	AvsStreamParser*			m_parser;
	int64_t						m_duration;
	AvsVideoView::Ptr 			m_view;
};

#endif	//