#pragma once

#include <mutex>
#include <list>
#include <ctime>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

#define FFMPEG_ERR_BUFF_LEN    256

#define INSTANCE_IMP(class_name, ...) \
class_name &class_name::Instance() { \
    static std::shared_ptr<class_name> s_instance(new class_name(__VA_ARGS__)); \
    static class_name &s_instance_ref = *s_instance; \
    return s_instance_ref; \
}

std::string makeRandStr(int sz, bool printable = true);

struct NotiData {
    enum NotiType {
        MEDIA = 0,
        STEPEND = 1,
        SETP_NOT_ENOUGH = 2,
    };
    NotiType    type;
    int         width;
    int         height;
    int         fps;
};

class IObserver {
public:
    virtual void update(NotiData notice) = 0;
};

class ISubject {
public:
    void addObserver(IObserver* ob) {
        if (ob) {
            m_observers.push_back(ob);
        }
    }

    void notify(NotiData data) {
        for (auto& ob : m_observers)
            ob->update(data);
    }

private:
    std::list<IObserver*>		m_observers;
};

#if defined(_WIN32)
    int gettimeofday(struct timeval* tp, void* tzp);
#endif

long getGMTOff();

//获取当前时间戳 毫秒
int64_t getNowMs();

double getRelativeTime();

void AVSleep(double sec);

std::time_t getTimeStamp();

/**
 * 获取时间字符串
 * @param fmt 时间格式，譬如%Y-%m-%d %H:%M:%S
 * @return 时间字符串
 */
std::string getTimeStr(const char* fmt, time_t time = 0);

/**
 * 根据unix时间戳获取本地时间
 * @param sec unix时间戳
 * @return tm结构体
 */
struct tm getLocalTime(time_t sec);

//字符串是否以xx开头
bool start_with(const std::string& str, const std::string& substr);
//字符串是否以xx结尾
bool end_with(const std::string& str, const std::string& substr);

class Task
{
public:
    Task() { }
    virtual ~Task() { }
    virtual int doTask(AVPacket* pkt) = 0;

    bool m_forward = false;
};