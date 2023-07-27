//CryptoReader的实现
// Path: crypto_reader.cpp

#include "crypto_reader.h"
#include <map>

#include "md5.h"
#include "util.h"
#include "once_token.h"
#include "as_log.h"
#if ((AS_APP_OS & AS_OS_WIN) == AS_OS_WIN)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_ANDROID)
#include "extend/as_frame.h"
#elif ((AS_APP_OS & AS_OS_ANDROID) == AS_OS_IOS)
#include "as_frame.h"
#endif

using namespace avs_toolkit;
extern const char* g_iv;
extern uint32_t g_idr_mark;
extern uint16_t g_payload_marker;
extern uint16_t g_frame_prefix;

static const char* signature = "AVS";

static int createStream(MK_Stream** stream, MK_CodecParameters** codecpar)
{
	if (!(*stream = AS_NEW(*stream)))
		return -1;

	memset(*stream, 0, sizeof(MK_Stream));
	(*stream)->index = -1;

	if (!(*codecpar = AS_NEW(*codecpar))) {
		AS_DELETE(*stream);
		return -1;
	}
	memset(*codecpar, 0, sizeof(MK_CodecParameters));
	(*stream)->codecpar = *codecpar;
	return 0;
}

#define GEN_STATUS(x,y)  MEDIA_STATUS_INFO status{ MR_CLIENT_STATUS(x), y }

#define PRINT_ERROR(err) AS_LOG(AS_LOG_WARNING, "read %s error, %s.", m_url.data(), err)

static std::map<int, const char*> err_map;
static OnceToken s_token([]() {
	err_map.emplace(-1, "read file failed");
	err_map.emplace(-2, "open file failed");
	err_map.emplace(-3, "signature mismatch");
	err_map.emplace(-4, "md5 mismatch, avs file may be tampered");
	err_map.emplace(-5, "unsupported encrpto algorithm");
	err_map.emplace(-6, "payload marker mismatch");
	err_map.emplace(-7, "no memeory");
	err_map.emplace(-8, "exceed limit days");
	err_map.emplace(-9, "exceed limit times");
	err_map.emplace(-10, "update header fail");
	err_map.emplace(-11, "not support none vidoe avs");
});

static const auto s_second_per_day = 24 * 60 * 60;

static uint64_t getDay(time_t second) {
	return (second + getGMTOff()) / s_second_per_day;
}

namespace crypto
{
	CryptoReader::CryptoReader():avs_ingress(),
		m_aes(nullptr), m_encrypt_buff(nullptr), m_format(nullptr)
	{
		m_mediaCb.ctx = this;
		m_mediaCb.m_cb_buffer = crypto_client_handlle_buffer;
		m_mediaCb.m_cb_data = crypto_client_handlle_media_data;
		m_mediaCb.m_cb_status = crypto_client_handlle_status;
		memset(&m_header, 0, sizeof(m_header));
		m_little_endian = is_little_endian();
	}

	CryptoReader::~CryptoReader() {
		close();
	}

#define CHECK_INTEGER(x)    if (!m_little_endian) { x = to_reverse_endian(x); }

	int CryptoReader::probe_ingress(CryptoHeader& header)
	{
		if (!m_file.open(m_url, false)) {
			return -1;
		}
		
		int ret = 0;
		if (!(m_file.read(0, std::ios::cur, m_header))) {
			return -1;
		}
		if (memcmp(signature, m_header.signature, 3) != 0) {
			return -2;
		}
		
		CHECK_INTEGER(m_header.create_time);
		CHECK_INTEGER(m_header.encry_algorithm);
		CHECK_INTEGER(m_header.limit_day);
		CHECK_INTEGER(m_header.use_times);
		CHECK_INTEGER(m_header.limit_times);
		CHECK_INTEGER(m_header.total_duration);
		CHECK_INTEGER(m_header.video_width);
		CHECK_INTEGER(m_header.video_height);
		CHECK_INTEGER(m_header.video_frame_rate);
		CHECK_INTEGER(m_header.video_codec);
		CHECK_INTEGER(m_header.audio_sample_rate);
		CHECK_INTEGER(m_header.audio_sample_bits);
		CHECK_INTEGER(m_header.audio_channels);
		CHECK_INTEGER(m_header.audio_codec);
		header = m_header;
		return 0;
	}

	int32_t CryptoReader::start_ingress(int16_t fragments, bool voice)
	{
		if (!m_file.open(m_url, false)) {
			return -1;
		}
		
		int ret = 0;
		if ((ret = parseHeader()) < 0) {
			GEN_STATUS(kCheckHeaderFaild, 401);
			m_mediaCb.m_cb_status(nullptr, status, this);
			return ret;
		}

		if(!(m_encrypt_buff = AS_NEW(m_encrypt_buff, 3 * 1024 * 1024))) {
			GEN_STATUS(kAllocResourceFaild, 402);
			m_mediaCb.m_cb_status(nullptr, status, this);
			return -1;
		}
		GEN_STATUS(kParsedMediaAttribute, 200);
		m_mediaCb.m_cb_status(nullptr, status, this);
		return ret;
	}

	int32_t CryptoReader::vcr_control(double start, double scale)
	{
		if (start < 0.0 || start > 1.0) {
			AS_LOG(AS_LOG_WARNING, "start must be in [0, 1], %f", start);
			return -1;
		}

		uint32_t marker = 0;
		if (!m_file.read(m_header.idr_fp, std::ios::beg, marker)) {
			PRINT_ERROR("locate idr marker failed");
			return -1;
		}

		if (g_idr_mark != marker) {
			PRINT_ERROR("idr marker mismatch,seek fail.");
			return -1;
		}

		uint32_t idrCount = 0;
		if (!(m_file.read(0, std::ios::cur, idrCount))) {
			PRINT_ERROR("read idr count");
			return -1;
		}
		
		int anchor = idrCount * start;
		if (anchor > idrCount) {
			anchor = idrCount;
		}
		
		int64_t pts = 0, target = m_format->duration * start;

		int ret = -1;
		do {
			if (!m_file.read(anchor * 16, std::ios::cur, pts)) {
				break;
			}

			while ((pts < target) && (anchor <= idrCount)) {
				++anchor;
				if (!m_file.read(8, std::ios::cur, pts)) {
					break;
				}
			}

			while ((pts > target) && (anchor >= 0)) {
				anchor--;
				if (!m_file.read(-24, std::ios::cur, pts)) {
					break;
				}
			}

			if (pts > target) {
				break;
			}
			ret = 0;
		} while (false);
	
		if (ret < 0){
			//PRINT_ERROR("%s, seek to position %lf failed", m_url.data(), start);
			return ret;
		}
		
		uint64_t offset = 0;
		if (!m_file.read(0, std::ios::cur, offset) || !m_file.seek(offset, std::ios::beg)) {
			return -1;
		}
		return 0;
	}

	int CryptoReader::parseHeader()
	{
		if (!(m_file.read(0, std::ios::cur, m_header))) {
			m_file.close();
			return -1;
		}

		int ret = 0;
		if (memcmp(signature, m_header.signature, 3) != 0) {
			ret = -3;
			PRINT_ERROR(err_map[ret]);
			m_file.close();
			return ret;
		}

		CHECK_INTEGER(m_header.create_time);
		CHECK_INTEGER(m_header.encry_algorithm);
		CHECK_INTEGER(m_header.limit_day);
		CHECK_INTEGER(m_header.use_times);
		CHECK_INTEGER(m_header.limit_times);
		CHECK_INTEGER(m_header.total_duration);
		CHECK_INTEGER(m_header.video_width);
		CHECK_INTEGER(m_header.video_height);
		CHECK_INTEGER(m_header.video_frame_rate);
		CHECK_INTEGER(m_header.video_codec);
		CHECK_INTEGER(m_header.audio_sample_rate);
		CHECK_INTEGER(m_header.audio_sample_bits);
		CHECK_INTEGER(m_header.audio_channels);
		CHECK_INTEGER(m_header.audio_codec);
		CHECK_INTEGER(m_header.idr_fp);

		if (m_header.limit_day <= 0 || m_header.limit_times <= 0) {
			ret = -8;
			PRINT_ERROR(err_map[ret]);
			m_file.close();
			return ret;
		}

		do {
			string reserved;
			reserved.append(to_string(m_header.create_time)).append(to_string(m_header.limit_day))
				.append(to_string(m_header.use_times)).append(to_string(m_header.limit_times))
				.append(string(m_header.encry_key, sizeof(m_header.encry_key))).append("allcam@2022");

			string md5_summary = avs_toolkit::MD5(reserved).hexdigest();
			AS_LOG(AS_LOG_INFO, "reader reserved:%s, md5:%s, header:%s.", reserved.data(), md5_summary.data(), m_header.md5);
			if (md5_summary != string(m_header.md5, sizeof(m_header.md5))) {
				ret = -4;
				break;
			}

			time_t now = time(nullptr);
			int day = (now - m_header.create_time) / s_second_per_day;
			if (day > m_header.limit_day) {
				ret = -8;
				break;
			}

			if (m_header.use_times >= m_header.limit_times) {
				ret = -9;
				break;
			}

			if (1 != m_header.encry_algorithm) {
				ret = -5;
				break;
			}

			if (!(m_format = AS_NEW(m_format))) {
				ret = -7;
				break;
			}

			m_format->duration = m_header.total_duration;
			MK_Stream* stream;
			MK_CodecParameters* codecpar;
			if (m_header.video_width > 0 && m_header.video_height > 0 && m_header.video_codec > 0) {
				if (createStream(&stream, &codecpar) < 0) {
					ret = -7;
					break;
				}

				stream->index = 0;
				stream->time_base.num = 1;
				stream->time_base.den = 1000;
				stream->codecpar->codec_type = MEDIA_TYPE_VIDEO;
				stream->codecpar->codec_id = (MKCodecID)m_header.video_codec;
				stream->codecpar->width = m_header.video_width;
				stream->codecpar->height = m_header.video_height;
				m_format->video_stream = 0;
				m_format->streams.emplace_back(std::move(stream));
			}
			else {
				ret = -11;
				break;
			}

			if (m_header.audio_sample_rate > 0 && m_header.audio_channels > 0 && m_header.audio_codec > 0) {
				if (createStream(&stream, &codecpar) < 0) {
					ret = -7;
					break;
				}

				stream->index = 1;
				stream->time_base.num = 1;
				stream->time_base.den = 1000;
				stream->codecpar->codec_type = MEDIA_TYPE_AUDIO;
				stream->codecpar->codec_id = (MKCodecID)m_header.audio_codec;
				stream->codecpar->channels = m_header.audio_channels;
				stream->codecpar->sample_rate = m_header.audio_sample_rate;
				m_format->audio_stream = 1;
				m_format->streams.emplace_back(std::move(stream));
			}

			uint16_t marker = 0;
			m_file.read(0, std::ios::cur, marker);
			CHECK_INTEGER(marker);
			if (g_payload_marker != marker) {
				ret = -6;
				break;
			}

			m_aes.reset(new AES256CBC(m_header.encry_key, g_iv));
			m_header.use_times++;
			//立即更新文件头信息，防止程序异常退出
			string reserved2;
			reserved2.append(to_string(m_header.create_time)).append(to_string(m_header.limit_day))
				.append(to_string(m_header.use_times)).append(to_string(m_header.limit_times))
				.append(string(m_header.encry_key, sizeof(m_header.encry_key))).append("allcam@2022");

			string md5 = avs_toolkit::MD5(reserved2).hexdigest();
			memcpy(m_header.md5, md5.data(), sizeof(m_header.md5));

			fstream ofs(m_url, ios::binary | ios::out | ios::in);
			ofs.seekp(0, ios::beg);
			if (!ofs.write((char*)&m_header, sizeof(m_header)).good()) {
				ret = -10;
				ofs.close();
				break;
			}
			ofs.close();
		}while (0);

		if (ret < 0) {
			PRINT_ERROR(err_map[ret]);
			m_file.close();
		}
		return ret;
	}

	int CryptoReader::readFrameData(int64_t &ptsRef, int& type, bool process)
	{
		if (m_paused) {
			return 0;
		}
		
		if (m_file.getCur() >= m_header.idr_fp) {	// eof
			return AVERROR_EOF;
		}
		
		uint8_t frame_type = 0;
		bool encrypt = 0;
		uint16_t prefix = 0;
		uint32_t frame_size = 0;
		uint64_t pts = 0;
		
		int ret = kFileIOError;
		do{
			if (!m_file.read(0, std::ios::cur, prefix)) {
				break;
			}
			CHECK_INTEGER(prefix);
			if (g_frame_prefix != prefix) {
				PRINT_ERROR("frame prefix mismatach");
				ret = kInvalidData;
				break;
			}

			if (!m_file.read(0, std::ios::cur, frame_type)) {
				break;
			}

			if (2 <= frame_type) {
				auto pos = m_file.getCur();
				char err_type[64] = { 0 };
				sprintf(err_type, "frame type %d error", frame_type);
				PRINT_ERROR(err_type);
				ret = kInvalidData;
				break;
			}

			if (!m_file.read(0, std::ios::cur, encrypt)) {
				break;
			}
			
			if (!m_file.read(0, std::ios::cur, frame_size)) {
				break;
			}
			CHECK_INTEGER(frame_size);
			
			if (!m_file.read(0, std::ios::cur, pts)) {
				break;
			}
			CHECK_INTEGER(pts);

			if (frame_size > (UINT32_MAX / 10)) {
				ret = kInvalidData;
				break;
			}
			ret = 0;
		} while (0);
	
		if (ret < 0) {
			PRINT_ERROR("read frame header failed");
			return ret;
		}
		
#ifdef  _DEBUG
		if (0 == frame_type) {
			AS_LOG(AS_LOG_INFO, "read avs raw data, pts:%lld.", pts);
		}
#endif //  _DEBUG

		uint32_t alloc_buff_len = 0;
		char* frame_data = m_mediaCb.m_cb_buffer(nullptr, frame_size, alloc_buff_len, this);
		if(alloc_buff_len < frame_size) {
			ret = kInvalidData;	//数据超过3MB,认为无效
			//PRINT_ERROR("frame size [%d] is too big,invalid.", frame_size);
			return ret;
		}
		
		auto file_size = m_header.idr_fp - m_file.getCur();
		if (file_size < frame_size) {
			ret = kReachAvaliable;
			PRINT_ERROR("file size is not enough too read.");
			return AVERROR_EOF;
		}

		int data_size = 0;
		if (!read(0, std::ios::cur, encrypt ? m_encrypt_buff : frame_data, frame_size)) {
			ret = kFileIOError;
			PRINT_ERROR("read frame data failed");
			return ret;
		}
		data_size = frame_size;
		ptsRef = pts;
		type = frame_type;
		if (!process) {
			return ret;
		}

		if (encrypt) {
			ret = m_aes->decrypt(m_encrypt_buff, data_size, frame_data);
			if(ret < 0) {
				ret = kDecryptFailed;
				PRINT_ERROR("decrypt failed.");
				return ret;
			}
			data_size = ret;
			ret = 0;
		}

		MediaDataInfo info;
		memset(&info, 0x0, sizeof(info));
		parseMediaInfo(frame_type, pts, frame_data, info);
		ret = m_mediaCb.m_cb_data(nullptr, &info, data_size, this);
		if (ret < 0) {
			ret = kParseFailed;
		}
		return ret;
	}

	int CryptoReader::close()
	{
		m_file.close();
		AS_DELETE(m_encrypt_buff, MULTI);
		if (m_format) {
			m_format->release();
			delete m_format;
		}
		return 0;
	}

	bool CryptoReader::read(size_t pos, std::ios_base::seekdir seek, char* data, size_t size)
	{
		if (!m_file.read(pos, seek, data, size)) {
			return false;
		}
		return true;
	}

	void CryptoReader::parseMediaInfo(int stIndex, uint32_t pts, char* buf, MediaDataInfo& dataInfo)
	{
		if (!m_format || stIndex < 0 || stIndex >= m_format->streams.size()) {
			return;
		}
		
		auto stream = m_format->streams.at(stIndex);
		dataInfo.codec_id = stream->codecpar->codec_id;
		dataInfo.code = MR_MEDIA_CODE_OK;
		dataInfo.is_key = 0;
		dataInfo.pts = pts;
		dataInfo.stream_index = stIndex;
		return;
	}
}
