#ifndef AVS_VIDEO_STATE_H__
#define AVS_VIDEO_STATE_H__

#include "util.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
}
    
#include "avs_player_factory.h"
#include "avs_pkt_frame_q.h"

class AvsDecoder;

class Clock
{
public:
    Clock() = default;

    void init(int* queue_serial) {
        this->queue_serial = queue_serial;
        speed = 1.0;
        paused = 0;
        setClock(NAN, -1);
    }

    void setClock(double pts, int serial) {
        double time = av_gettime_relative() / 1000000.0;
        setClockAt(pts, serial, time);
    }

    void setClockSpeed(double sped) {
        setClock(getClock(), this->serial);
        this->speed = sped;
    }

    double getClock() {
        if (!queue_serial || *queue_serial != this->serial)
            return NAN;

        if (paused) {
            return this->pts;
        }
        else {
            double time = av_gettime_relative() / 1000000.0;
            return this->pts_drift + time - (time - this->last_updated) * (1.0 - this->speed);
        }
    }

    void sync2Slave(Clock* slave) {
        double clock = this->getClock();
        double slave_clock = slave->getClock();
        static const double kNoSyncThreshold = 10.0;
        if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > kNoSyncThreshold)) {
            this->setClock(slave_clock, slave->getSerial());
        }
    }

    int getSerial() { return this->serial; }

    double getSpeed() { return this->speed; }

private:
    void setClockAt(double pts, int serial, double time) {
        this->pts = pts;
        this->last_updated = time;
        this->pts_drift = this->pts - time;
        this->serial = serial;
    }

public:
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    int serial = -1;
    double speed = 1.0;
    uint8_t paused = 0;
    int* queue_serial = nullptr;
};

enum PlayModeEnum {
    kRtsp = 1,
    kNormal = 2,
    kInvalid = 3,
};

enum {
    kSyncAudioMaster, /* default choice */
    kSyncVideoMaster,
    kSyncExternalClock, /* synchronize to an external clock */
};

typedef struct AudioParams {
    int freq;
    int channels;
    int64_t channel_layout;
    enum AVSampleFormat fmt;
    int frame_size;
    int bytes_per_sec;
} AudioParams;

class VideoState {
public:
    VideoState(long i, int tp) :id(i), type(tp) {
        videoq = make_shared<PacketQueue>();
        audioq = make_shared<PacketQueue>();

        vidclk.init(&videoq->m_serial);
        //ic->extclk.init(&ic->extclk.serial);
        extclk.init(&videoq->m_serial);
    }

    virtual ~VideoState() {
        swr_free(&this->swr_ctx);
        av_freep(&this->audio_buf1);
        this->audio_buf1_size = 0;
    }

    long    id;
    int     type;
    int     abort_request = 0;
    int     force_refresh = 0;
    int     paused = 0;
    int     last_paused = 0;
    int     read_pause_return = 0;
    int     seek_req = 0;
    int     seek_flags = 0;
    int64_t seek_pos = 0;
    int64_t seek_rel = 0;

    std::mutex              full_mutex;
    std::condition_variable full_cond;

    PlayModeEnum play_mode;
    int pre_cache = 1;
    int cached_enough = 0;
    double speed = 1.0;

    AVFormatContext* ic = nullptr;
    int realtime = 0;

    Clock   audclk;
    Clock   vidclk;
    Clock   extclk;

    FrameQueue::Ptr pictq;
    FrameQueue::Ptr sampq;

    AvsDecoder* viddec = nullptr;
    AvsDecoder* auddec = nullptr;

    int av_sync_type = 0;

    int audio_stream = -1;
    AVStream* audio_st = nullptr;
    PacketQueue::Ptr audioq;
    int audio_volume = 0;
    int muted = 0;

    struct AudioParams audio_src;
    struct AudioParams audio_tgt;
    struct SwrContext* swr_ctx = nullptr;
    uint8_t* audio_buf1 = nullptr;
    unsigned int audio_buf1_size = 0;
    
    double frame_timer = 0.0;
    int video_stream = -1;
    AVStream* video_st = nullptr;
    PacketQueue::Ptr videoq;
    double max_frame_duration = 8.0;      // maximum duration of a frame

    int eof = 0;
    int width = 0, height = 0;
    int step = 0;

    std::condition_variable continue_read_cond;

    int getMasterSyncType() {
        if (kRtsp == play_mode) {
            return kSyncExternalClock;
        }

        if (kSyncVideoMaster == av_sync_type) {
            if (video_stream >= 0)
                return kSyncVideoMaster;
            else
                return kSyncAudioMaster;
        }
        else if (kSyncAudioMaster == av_sync_type) {
            if (audio_stream >= 0)
                return kSyncAudioMaster;
            else
                return kSyncExternalClock;
        }
        else {
            return kSyncExternalClock;
        }
    }

    double getMasterClock() {
        double val;
        switch (getMasterSyncType()) {
        case kSyncVideoMaster:
            val = vidclk.getClock();
            break;
        case kSyncAudioMaster:
            val = audclk.getClock();
            break;
        default:
            val = extclk.getClock();
            break;
        }
        return val;
    }

    static double mapSpeed(double scale1) {  
        static double eps = 0.001;
        if (abs(scale1 - 252.0) < eps) {
            return -1.0;
        }
        else if (abs(scale1 - 253.0) < eps) {
            return  -2.0;
        }
        return scale1;
    }

    double vpDuration(MyFrame::Ptr vp, MyFrame::Ptr nextvp) {
        if (vp->serial == nextvp->serial) {
            double duration = nextvp->pts - vp->pts;
            if (isnan(duration) || (mapSpeed(this->speed) * duration <= 0.0) || duration > max_frame_duration)
                return vp->duration;
            else
                return duration;
        }
        else {
            return 0.0;
        }
    }

    void sendStatus(long status, std::string description) {
        avs_player_factory::GetInstance()->sendStatusMsg(id, type, status, description);
    }

    void sendData(long progress, long total) {
        avs_player_factory::GetInstance()->sendDataMsg(id, type, progress, total);
    }
};

#endif // ! AVS_VIDEO_STATE_H__
