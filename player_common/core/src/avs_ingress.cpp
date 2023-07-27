#include "avs_ingress.h"
#include "as_common.h"
#include "util.h"
#include "crypto_reader.h"
#include "avs_demuxer.h"

using namespace crypto;

int kULimit = UINT32_MAX / 2;
static const char* kAVSSuffix = ".avs";
    
int32_t avs_ingress::init(std::string strUrl, avs_ingress_data* ingressData) {
	m_ingreeData = ingressData;
	m_url = strUrl;
    return 0;
}

std::string avs_ingress_prober::probe_url(const std::string& url) {
	string url_info;
	int ret = 0;
	if (end_with(url, kAVSSuffix)) {
		auto root = cJSON_CreateObject();
		if (!root) {
			AS_LOG(AS_LOG_ERROR, "cJSON_CreateObject failed!");
			return url_info;
		}
		CryptoReader reader;
		CryptoHeader header;
		reader.init(url.c_str(), nullptr);
		ret = reader.probe_ingress(header);
		if(ret >= 0){		
			auto format = cJSON_CreateString("avs");
			cJSON_AddItemToObject(root, "format", format);

			auto encrypt = cJSON_CreateNumber(1);
			cJSON_AddItemToObject(root, "encrypted", encrypt);
			
			if (header.limit_times < kULimit) {
				auto remaining_times = cJSON_CreateNumber(header.limit_times - header.use_times);
				cJSON_AddItemToObject(root, "remaining_times", remaining_times);
			}

			if (header.limit_day < kULimit) {
				time_t ts = header.create_time + header.limit_day * 24 * 60 * 60;
				auto deadline = cJSON_CreateString(getTimeStr("%Y-%m-%d", ts).data());
				cJSON_AddItemToObject(root, "deadline", deadline);
			}

			auto duration = cJSON_CreateNumber(header.total_duration/ 1000);
			cJSON_AddItemToObject(root, "duration", duration);
		}
		else {
			AS_LOG(AS_LOG_ERROR, "CryptoReader probe avs failed, %d!", ret);
			auto format = cJSON_CreateString("invalid avs");
			cJSON_AddItemToObject(root, "format", format);
		}
		auto info_str = cJSON_Print(root);
		url_info = std::string(info_str);
		free(info_str);
		cJSON_Delete(root);
	}
	else {
		url_info = AvsDemuxer::probeUrl(url);
	}
	return url_info;
}
