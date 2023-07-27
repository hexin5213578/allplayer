#include "crypto_writer.h"
#include "as_file.h"
#include "md5.h"
#include "as_base64.h"
#include "as_log.h"

extern "C"
{
#include <libavformat/avformat.h>
}

using namespace std;
using namespace avs_toolkit;
extern const char* g_iv;
extern uint32_t g_idr_mark;
extern uint16_t g_payload_marker;
extern uint16_t g_frame_prefix;

static const int kDeltaThreshold = 2500;

namespace crypto
{
	CryptoWriter::CryptoWriter(const string& path, const string& key, const int limit_days, const int limit_times)
		:m_path(path), m_key(key), m_aes(key.data(), g_iv),
		m_limitDays(limit_days), m_limitTimes(limit_times),
		m_cipherBuffer(nullptr)
	{
		m_lastVPts = INT64_MIN;
		m_vidPts = m_audPts = 0;
		m_duration = 0;
		m_delta = 40;
		memset(&m_header, 0, sizeof(m_header));
	}

	CryptoWriter::~CryptoWriter() {
		close();
	}
	
	bool CryptoWriter::open()
	{
		if (m_path.empty()) {
			throw std::runtime_error("crypto file path must be set");
		}
		
		bool ret = false;
		string dir = as_file::parentDir(m_path);

#if !defined(_WIN32)
		//创建文件夹
		ret = as_file::create_path(dir.data(), S_IRWXO | S_IRWXG | S_IRWXU);
#else
		ret = as_file::create_path(dir.data(), 0);
#endif
		
		ret = m_file.open(m_path, true);
		if (!ret) {
			return ret;
		}
		
		m_cipherBuffer = new char[2 * 1024 * 1024];
		return nullptr != m_cipherBuffer;
	}

#define CHECK_INTEGER(x, y)    x = y; \
			if (!is_little_endian()) {x = to_reverse_endian(x);}  \

	int CryptoWriter::generateHeader(MK_Format_Contex* format)
	{
		memcpy(m_header.signature, "AVS", 3);
		
		m_header.version = 0x10;
		CHECK_INTEGER(m_header.header_len, sizeof(m_header));
		CHECK_INTEGER(m_header.encry_algorithm, 1);
		CHECK_INTEGER(m_header.limit_day, m_limitDays);
		m_header.use_times = 0;
		CHECK_INTEGER(m_header.limit_times, m_limitTimes);
		memcpy(m_header.encry_key, m_key.data(), sizeof m_header.encry_key);
		
		//CHECK_INTEGER(m_header.total_duration, 0);
		
		if (format->video_stream >= 0) {
			CHECK_INTEGER(m_header.video_width, format->streams[format->video_stream]->codecpar->width);
			CHECK_INTEGER(m_header.video_height, format->streams[format->video_stream]->codecpar->height);
			CHECK_INTEGER(m_header.video_frame_rate, (uint32_t)format->streams[format->video_stream]->codecpar->video_fps);
			CHECK_INTEGER(m_header.video_codec, format->streams[format->video_stream]->codecpar->codec_id);
		}

		if (format->audio_stream >= 0) {
			CHECK_INTEGER(m_header.audio_sample_rate, format->streams[format->audio_stream]->codecpar->sample_rate);
			CHECK_INTEGER(m_header.audio_sample_bits, format->streams[format->audio_stream]->codecpar->format);
			CHECK_INTEGER(m_header.audio_channels, format->streams[format->audio_stream]->codecpar->channels);
			CHECK_INTEGER(m_header.audio_codec, format->streams[format->audio_stream]->codecpar->codec_id);
		}
		
		uint16_t payload_mark;
		CHECK_INTEGER(payload_mark, g_payload_marker);
		if (!m_file.writeAtPos(sizeof(m_header), (char*)&payload_mark, sizeof(payload_mark))) {
			return -1;
		}
		return 0;
	}

	bool CryptoWriter::writeFrameData(int type, AVPacket* pkt, bool encrypt)
	{
		uint32_t final_size = pkt->size;
		char* final_data = (char*)pkt->data;
		bool ret = true;
		
		auto frame_rate = is_little_endian ? m_header.video_frame_rate : to_reverse_endian(m_header.video_frame_rate);
		if (0 == frame_rate) {
			frame_rate = m_delta;
		}
		int duration = 1000 / frame_rate;

		if (0 == type) {
			if (INT64_MIN == m_lastVPts) {
				//first frame
			}
			else if (m_lastVPts != pkt->pts) {
				auto delta = pkt->pts - m_lastVPts;
				if (delta > kDeltaThreshold || delta < 0) {
					delta = m_delta;
				}
				else if (delta < m_delta) {
					m_delta = delta;
				}
			
				m_vidPts += delta;
			}
			m_lastVPts = pkt->pts;
			pkt->pts = m_vidPts;
			m_duration = pkt->pts + 40;
		}
		else if(1 == type) {
			auto delta = pkt->pts - m_audPts;
			delta = delta >= kDeltaThreshold ? duration : delta;
			m_audPts += delta;
			pkt->pts = m_audPts;
		}

#ifdef _DEBUG
		AS_LOG(AS_LOG_INFO, "%s store with pts %lld.", type == 0 ? "video" : "audio", pkt->pts);
#endif //  _DEBUG

		do {
			if (encrypt) {
				// 根据加密前数据的大小, 计算加密后数据的大小
				int encrypt_size = m_aes.getAES256CBCSize(final_size);
				int size = m_aes.encrypt(final_data, final_size, m_cipherBuffer);
				if (size < 0 || size != encrypt_size) {
					ret = false;
					break;
				}
				final_data = m_cipherBuffer;
				final_size = size;
			}

			uint64_t pos = 0;
			if (pkt->flags & AV_PKT_FLAG_KEY) {
				pos = m_file.getCur();
			}

			CHECK_INTEGER(g_frame_prefix, g_frame_prefix);
			if (!(ret = write((char*)&g_frame_prefix), sizeof(g_frame_prefix))) {
				break;
			}

			uint8_t type1 = type;
			if (!(ret = write((char*)&type1, sizeof(type1))))
				break;

			if (!(ret = write((char*)&encrypt, sizeof(encrypt))))
				break;

			CHECK_INTEGER(final_size, final_size);
			if (!(ret = write((char*)&final_size, sizeof(final_size))))
				break;

			uint64_t pts = pkt->pts;
			CHECK_INTEGER(pts, pts);
			if (!(ret = write((char*)&pts, sizeof(pts))))
				break;

			//写入数据
			if (!(ret = write(final_data, final_size)))
				break;
			
			if (pos) {
				m_idrInfoMap[pkt->pts] = pos;
			}
			m_file.flush();
		} while (0);
		
		if (!ret) {
		}
		return ret;
	}

	bool CryptoWriter::writeTail()
	{		
		m_file.seekAnchor(1);
		auto fend = m_file.getCur();
		AS_LOG(AS_LOG_INFO, "seek at end of file, %lld.", fend);
		CHECK_INTEGER(m_header.idr_fp, fend);

		uint32_t mark = g_idr_mark;
		//写入mark
		if (!write((char*)&mark, sizeof(mark))) {
			return false;
		}
		//写入IDR信息
		uint32_t idr_count = m_idrInfoMap.size();
		CHECK_INTEGER(idr_count, idr_count);
		if (!write((char*)&idr_count, sizeof(idr_count))) {
			return false;
		}
		
		for (auto& idr : m_idrInfoMap) {
			uint64_t val; 
			CHECK_INTEGER(val, idr.first);
			if (!write((char*)&val, sizeof(val))) {
				return false;
			}
			
			CHECK_INTEGER(val, idr.second);
			if (!write((char*)&val, sizeof(val))) {
				return false;
			}
		}

		time_t now = time(nullptr);
		CHECK_INTEGER(m_header.create_time, now);
		CHECK_INTEGER(m_header.total_duration, m_duration);
		string reserved;
		reserved.append(to_string(m_header.create_time)).append(to_string(m_header.limit_day))
			.append(to_string(m_header.use_times)).append(to_string(m_header.limit_times))
			.append(m_key).append("allcam@2022");
	
		string md5_summary = avs_toolkit::MD5(reserved).hexdigest();
		AS_LOG(AS_LOG_INFO, "writer reserved:%s. md5:%s.", reserved.data(), md5_summary.data());
		memcpy(m_header.md5, md5_summary.data(), sizeof(m_header.md5));
		m_file.writeAtPos(0, (char*)&m_header, sizeof(m_header));
		return true;
	}

	int CryptoWriter::close()
	{
		m_file.close();
		return 0;
	}

	bool CryptoWriter::write(const std::string& str)
	{
		return m_file.write<char>(str.data(), str.size());
	}
	
	bool CryptoWriter::write(const char* data, size_t size)
	{
		return m_file.write<char>(data, size);
	}
}
