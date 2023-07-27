#ifndef CRYPTO_WRTIER_H__
#define CRYPTO_WRTIER_H__

#include <map>

#include "avs_player_common.h"
#include "filerw.h"
#include "aes256_cbc.h"
#include "crypto_util.h"

struct AVPacket;

namespace crypto
{
	class CryptoWriter
	{
	public:
		CryptoWriter(const std::string& path, const std::string& key, const int limit_days, const int limit_times);
		virtual ~CryptoWriter();

		bool open();

		int generateHeader(MK_Format_Contex* format);

		//写入帧数据,encrypt是否需要加密,返回写入的字节数
		bool writeFrameData(int type, AVPacket* pkt, bool encrypt);

		//写文件尾
		bool writeTail();

		uint64_t getFileSize() { return m_file.size(); }
		int64_t getDuration() { return m_duration; }

	protected:
		int close();
		bool write(const std::string& str);
		bool write(const char* data, size_t size);

	private:
		std::string						m_path;
		avs_toolkit::FileRW				m_file;
		CryptoHeader					m_header;

		std::string						m_key;
		avs_toolkit::AES256CBC			m_aes;
		uint32_t						m_limitDays;
		uint32_t						m_limitTimes;

		uint64_t						m_duration;
		uint64_t						m_idrPos;
		std::map<uint64_t, uint64_t>	m_idrInfoMap;
		char*							m_cipherBuffer;

		int64_t							m_lastVPts;
		int64_t							m_vidPts;
		int64_t							m_audPts;
		int								m_delta;
	};
}

#endif


