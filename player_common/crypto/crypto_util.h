#pragma once 
#include <cstdint>
#include <algorithm>

//判断是否为小端字节序
static bool is_little_endian()
{
	uint16_t x = 0x0001;
	uint8_t y = *reinterpret_cast<uint8_t*>(&x);
	return y == 1;
}

template <typename T>
T to_reverse_endian(T x)
{
	static_assert(std::is_unsigned<T>::value, "T must be an unsigned integer type");
	static_assert(sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "T must be a 16-, 32-, or 64-bit integer type");

	// Reverse the byte order
	T y = 0;
	for (std::size_t i = 0; i < sizeof(T); ++i) {
		y <<= 8;
		y |= x & 0xff;
		x >>= 8;
	}
	return y;
}

enum AvsStatusEnum {
	kCheckHeaderFaild = 1, 		//AVS格式文件头校验失败，无效的avs文件
	kParsedMediaAttribute = 2, 		//读取到媒体属性
	kAllocResourceFaild = 3,	//分配资源失败
	kMax = 0xFF					
};

enum AvsErrorCode {
	kNoError = 0,				//无错误
	kFileIOError = -1,			//文件IO错误
	kInvalidData = -2,			//文件格式错误
	kReachAvaliable = -3,		//文件不够读
	kDecryptFailed = -4,		//解密失败
	kParseFailed = -5,			//解析失败
};

#pragma pack(push,1)
struct CryptoHeader
{
	char		signature[3];
	uint8_t		version;
	uint32_t	header_len;
	uint64_t	create_time;
	uint32_t	encry_algorithm;
	uint32_t	limit_day;
	uint32_t	use_times;
	uint32_t	limit_times;
	char		encry_key[32];
	char		md5[32];
	uint64_t	total_duration;
	uint32_t	video_width;
	uint32_t	video_height;
	uint32_t	video_frame_rate;
	uint32_t	video_codec;
	uint32_t	audio_sample_rate;
	uint32_t	audio_sample_bits;
	uint32_t	audio_channels;
	uint32_t	audio_codec;
	uint64_t	idr_fp;
};
#pragma pack(pop)