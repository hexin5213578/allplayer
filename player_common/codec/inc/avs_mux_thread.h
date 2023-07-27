#pragma once

#include <memory>

#include "avs_thread_base.h"
#include "avs_egress.h"
#include "avs_player.h"
#include "avs_rtsp_processor.h"

class AvsMuxThread : public AvsPlayer, public AvsThreadBase, public AvsRtspProcessor
{
public:
    AvsMuxThread(BizParams& p);
    virtual ~AvsMuxThread();

    int play() override;

    void pause(bool pause, bool clear) override;

    int setMediaAttribute(MK_Format_Contex* format) override;

    int doTask(AVPacket* pkt) override;

	bool init();

    void stopRun() override;

    void stop() override { stopRun(); }
    
    bool isRecording() { return !m_bIsExit; }

    void handleClose() override;

    int vcrControl(double start, double scale, int option);

protected:
	void mainProcess() override;

    int releaseMuxer();

private:
    std::string genFileName();
    std::string getMP4FileName();
    std::string getAVSFileName();

    int32_t checkNeedCutting(uint64_t ulCurPts);

    inline bool isDownloadBiz() {
        return TYPE_DOWNLOAD_START == this->type;
    }

private:
    pktsCache           m_pkts;

    std::string         m_output;
    avs_egress*         m_muxer = nullptr;
    MK_Format_Contex*   m_format = nullptr;
   
    int64_t             m_startAudioPts = -1;
    int64_t  		    m_startVideoPts = -1;

    std::string         m_szCameraId;  
	FileDumpSt          m_dumpSt;
    int                 m_currentFileNum = 0;	                //当前切片第几段文件
    uint64_t            m_lastRecordPts = 0;                   //分片起始位置(视频)
    uint64_t            m_lastPostData = UINT64_MAX;                   //下载的当前秒数
    uint64_t            m_dumpedSize = 0;                   //下载数据大小(Byte)
	bool                m_waitedKeyFrame = false;
};

