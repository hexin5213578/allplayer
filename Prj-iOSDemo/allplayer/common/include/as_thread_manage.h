#ifndef __AS_THREAD_MANAGE_INCLUDE_H__
#define __AS_THREAD_MANAGE_INCLUDE_H__

#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
extern "C"{
#include  "as_mutex.h"
#include  "as_thread.h"
}

#define AS_THREAD_STACK_DEFAULT       (128*1024) 

#include <list>

#define REPORT_THREAD_STATUS_INTERVAL                10

#define MULTIPLE_OF_REPORT_THREAD_STATUS_INTERVAL    3

#define THREAD_NUM_EXCEPT_TASK                       0
#define COMMON_BUFFER_SIZE                           512

#define REFRESH_CONFIG_INTERVAL                      60


enum CHECK_INTERVAL
{
    MIN_INTERVAL                   = 180,
    MAX_INTERVAL                   = 3600,
    DEFAULT_INTERVAL               = 1800,
    DEFAULT_REPORT_INTERVAL        = 10,
    DEFAULT_THREAD_CHECK_INTERVAL  = 60
};


#define MAX_THREAD_NAME                64


#define MAX_THREAD_NUM                 512


class as_thread_reporter
{
public:
    as_thread_reporter(const char *pszThreadName);

    virtual ~as_thread_reporter();

    time_t getLastReportTime()const;

    uint32_t getProcessNum()const;

    void ReportStat(uint32_t ulProcessNum = 1);

private:
    as_thread_reporter();

    int32_t             m_nThreadIndex;

    time_t              m_tvLastReportTime;

    uint32_t            m_ulProcessNum;
};

class as_task_manage
{
public:
    static as_task_manage* instance()
    {
        static as_task_manage svsDaemonThread;
        return &svsDaemonThread;
    }

    virtual ~as_task_manage();

    int32_t Init(uint32_t ulMaxCheckInterval = DEFAULT_INTERVAL,
             uint32_t ulRestartServer = 0,
             uint32_t ulCoreDump = 0);
    void Destroy();

    int32_t RegistThread(as_thread_reporter* pReporter,const char* pszThreadName);

    int32_t UnregistThread(int32_t nThreadIndex);

    void    close();

    void    svc(void);

    int32_t open();
protected:
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    static  void* deamon_monitor_thread(void *arg);
#elif AS_APP_OS == AS_OS_WIN32
    static  uint32_t __stdcall deamon_monitor_thread(void *arg);
#endif
private:
    typedef struct tagThreadInfo
    {
        int32_t             m_nThreadIndex;
        uint32_t            m_ulThreadID;
        uint32_t            m_ulProcessNum;
        char                m_szThreadName[MAX_THREAD_NAME + 1];
        time_t              m_tvStartTime;
        time_t              m_tvAliveTime;
        as_thread_reporter* m_pReporter;
    }ThreadInfo;

    typedef std::list<int32_t>            FreeIndexList;
    typedef std::list<int32_t>::iterator  FreeIndexListIter;

private:
    as_task_manage();

    volatile bool     m_nThreadFlag;
    as_thread_t*      m_threadHandle;
    uint32_t          m_ulMaxThreadCheckInterval;
    uint32_t          m_ulRestartServer;
    uint32_t          m_ulCoreDump;

    ThreadInfo        m_ThreadArray[MAX_THREAD_NUM];

    as_mutex_t*       m_FreeIndexListMutex;
    FreeIndexList     m_FreeIndexList;

    time_t            m_tvLastCheckTime;
    int32_t           m_RegisteredThreadNum;
};

#endif /*__AS_THREAD_MANAGE_INCLUDE_H__*/