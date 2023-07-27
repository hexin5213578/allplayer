//加密读写类，使用FileRW类来读写文件，AES256CBC类来加密解密数据

#ifndef CRYPTO_REDDER_H__
#define CRYPTO_REDDER_H__

#include "filerw.h"
#include "aes256_cbc.h"
#include "avs_ingress.h"
#include "crypto_util.h"

namespace crypto
{
	class CryptoReader: public avs_ingress
	{
	public:
		using Ptr = std::shared_ptr<CryptoReader>;

		CryptoReader();
		virtual ~CryptoReader();
	
		const CryptoHeader getHeader() const { return m_header; }
		int probe_ingress(CryptoHeader& header);

		int32_t start_ingress(int16_t fragments = 1, bool voice = false) override;
		void stop_ingress() override { }
		void pause_ingress() override { m_paused = !m_paused; }
		int32_t vcr_control(double start, double scale) override;
		int32_t get_stream_stat(RTP_PACKET_STAT_INFO& stat) override { return 0; }
		MK_Format_Contex* get_format_context() override	{ return m_format; }
		
		//读取帧数据
		int readFrameData(int64_t& ptsRef, int& type, bool process = true);

	protected:
		int parseHeader();
		int close();
		bool read(size_t pos, std::ios_base::seekdir seek, char* data, size_t size);

		int handle_status(MR_CLIENT client, MEDIA_STATUS_INFO status, void* ctx) {
			return m_ingreeData->handle_ingress_status(status);
		}
		
		int handle_media_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len, void* ctx) {
			return m_ingreeData->handle_ingress_data(client, dataInfo, len);
		}

		char* handle_buffer(MR_CLIENT client, uint32_t len, uint32_t& ulBufLen, void* ctx) {
			return m_ingreeData->alloc_ingress_data_buf(len, ulBufLen);
		}
		
		static int crypto_client_handlle_status(MR_CLIENT client, MEDIA_STATUS_INFO status, void* ctx) {
			CryptoReader* reader = (CryptoReader*)ctx;
			return reader->handle_status(client, status, ctx);
		}

		static int crypto_client_handlle_media_data(MR_CLIENT client, MediaDataInfo* dataInfo, uint32_t len, void* ctx) {
			CryptoReader* reader = (CryptoReader*)ctx;
			return reader->handle_media_data(client, dataInfo, len, ctx);
		}

		static char* crypto_client_handlle_buffer(MR_CLIENT client, uint32_t len, uint32_t& ulBufLen, void* ctx) {
			CryptoReader* reader = (CryptoReader*)ctx;
			return reader->handle_buffer(client, len, ulBufLen, ctx);
		}

		void parseMediaInfo(int stIndex, uint32_t pts, char* buf, MediaDataInfo& dataInfo);

	private:
		MEDIA_CALL_BACK				m_mediaCb;
		avs_toolkit::FileRW			m_file;
		avs_toolkit::AES256CBC::Ptr	m_aes;
		CryptoHeader				m_header;
		char*						m_encrypt_buff;
		MK_Format_Contex*			m_format;

		bool						m_little_endian;
		bool						m_paused = false;
	};
}

#endif
