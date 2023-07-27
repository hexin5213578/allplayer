#include "util.h"

#include <random>

#include "as_config.h"
#include "once_token.h"

const int ERR_STR_SIZE = 512;
const int WAIT_TIME_MS = 1000;

static constexpr char CCH[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

std::string makeRandStr(int sz, bool printable) {
    std::string ret;
    ret.resize(sz);
    std::mt19937 rng(std::random_device{}());
    for (int i = 0; i < sz; ++i) {
        if (printable) {
            uint32_t x = rng() % (sizeof(CCH) - 1);
            ret[i] = CCH[x];
        }
        else {
            ret[i] = rng() % 0xFF;
        }
    }
    return ret;
}

int gettimeofday(timeval* tp, void* tzp) {
    auto now_stamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    tp->tv_sec = (decltype(tp->tv_sec))(now_stamp / 1000000LL);
    tp->tv_usec = now_stamp % 1000000LL;
    return 0;
}

static long s_gmtoff = 0; //时间差
static avs_toolkit::OnceToken s_token([]() {
#ifdef _WIN32
    TIME_ZONE_INFORMATION tzinfo;
    DWORD dwStandardDaylight;
    long bias;
    dwStandardDaylight = GetTimeZoneInformation(&tzinfo);
    bias = tzinfo.Bias;
    if (dwStandardDaylight == TIME_ZONE_ID_STANDARD) {
        bias += tzinfo.StandardBias;
    }
    if (dwStandardDaylight == TIME_ZONE_ID_DAYLIGHT) {
        bias += tzinfo.DaylightBias;
    }
    s_gmtoff = -bias * 60; //时间差(分钟)
#else
    //TODO local_time_init();
    s_gmtoff = getLocalTime(time(nullptr)).tm_gmtoff;
#endif // _WIN32
    });

long getGMTOff() {
    return s_gmtoff;
}

int64_t getNowMs() {
    int64_t microtime = av_gettime_relative();
    return microtime / 1000;
}

double getRelativeTime() {
    return av_gettime_relative() / 1000000.0;
}

void AVSleep(double sec) {
    av_usleep((int64_t)(sec * 1000000.0));
}

std::time_t getTimeStamp() {
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = 
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
   // return tp.time_since_epoch().count();
    return std::chrono::system_clock::to_time_t(tp);
}

std::string getTimeStr(const char* fmt, time_t time) {
    if (!time) {
        time = ::time(nullptr);
    }
    auto tm = getLocalTime(time);
    size_t size = strlen(fmt) + 64;
    std::string ret;
    ret.resize(size);
    size = std::strftime(&ret[0], size, fmt, &tm);
    if (size > 0) {
        ret.resize(size);
    }
    else {
        ret = fmt;
    }
    return ret;
}

struct tm getLocalTime(time_t sec) {
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &sec);
#else
    //TODO no_locks_localtime(&tm, sec);
#endif //_WIN32
    return tm;
}

bool start_with(const std::string& str, const std::string& substr) {
    return str.find(substr) == 0;
}

bool end_with(const std::string& str, const std::string& substr) {
    auto pos = str.rfind(substr);
    return pos != std::string::npos && pos == str.size() - substr.size();
}
