#include "as_thread_manage.h"
#include "as_lock_guard.h"
#include "as_log.h"
#include "as_time.h"
#include <time.h>

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#endif


as_task_manage::as_task_manage()
{
    m_threadHandle             = NULL;
    m_ulMaxThreadCheckInterval = DEFAULT_THREAD_CHECK_INTERVAL;
    m_ulRestartServer          = 0;
    m_ulCoreDump               = 0;
    m_nThreadFlag              = false;
    m_FreeIndexListMutex       = NULL;
    m_tvLastCheckTime          = time(NULL);
    m_RegisteredThreadNum      = 0;
}

as_task_manage::~as_task_manage()
{
    try
    {
        m_FreeIndexList.clear();
    }
    catch(...)
    {
    }

    if(NULL != m_FreeIndexListMutex) {
        as_destroy_mutex(m_FreeIndexListMutex);
        m_FreeIndexListMutex = NULL;
    }

    if(NULL != m_threadHandle) {
        as_destory_thread(m_threadHandle);
        m_threadHandle = NULL;
    }
}

int32_t as_task_manage::Init(uint32_t ulMaxCheckInterval,
                             uint32_t ulRestartServer,
                             uint32_t ulCoreDump)
{
    m_tvLastCheckTime          = time(NULL);
    m_ulMaxThreadCheckInterval = ulMaxCheckInterval;
    m_ulRestartServer          = ulRestartServer;
    m_ulCoreDump               = ulCoreDump;

    if ((MIN_INTERVAL > m_ulMaxThreadCheckInterval)
          || (MAX_INTERVAL < m_ulMaxThreadCheckInterval))
    {
        m_ulMaxThreadCheckInterval = DEFAULT_INTERVAL;
    }

    m_FreeIndexListMutex = as_create_mutex();
    if(NULL == m_FreeIndexListMutex) {
        return AS_ERROR_CODE_SYS;
    }


    ThreadInfo *  pReporter = NULL;
    as_lock_guard locker(m_FreeIndexListMutex);
    for (int32_t i = 0; i < MAX_THREAD_NUM; i++)
    {
        pReporter = m_ThreadArray + i;
        pReporter->m_ulThreadID      = 0;
        pReporter->m_nThreadIndex    = 0;
        pReporter->m_ulProcessNum    = 0;
        memset(pReporter->m_szThreadName, 0x0, MAX_THREAD_NAME + 1);
        pReporter->m_pReporter = NULL;
        m_FreeIndexList.push_back(i);
    }

    int32_t nRet = open();
    if (0 != nRet)
    {
        AS_LOG(AS_LOG_ERROR, "[deamon thread]Init SVS_Daemon_Thread fail.");
        return nRet;
    }

    AS_LOG(AS_LOG_INFO, "[deamon thread]Init SVS_Daemon_Thread success.");
    return AS_ERROR_CODE_OK;
}


void as_task_manage::Destroy()
{
    as_lock_guard locker(m_FreeIndexListMutex);
    m_FreeIndexList.clear();

    close();
    AS_LOG(AS_LOG_INFO, "[deamon thread]Destroy SVS_Daemon_Thread success.");
}


int32_t as_task_manage::RegistThread(as_thread_reporter* pReporter,const char* pszThreadName)
{
    if((NULL == pReporter) || (NULL == pszThreadName))
    {
        AS_LOG(AS_LOG_ERROR,
            "Thread Regist to Daemon failed.The parameter is invalid."
            "pReporter[0x%08x] pszThreadName[0x%08x]",
            pReporter,
            pszThreadName);

        return -1;
    }
    int32_t nThreadIndex = 0;
    {
        as_lock_guard locker(m_FreeIndexListMutex);
        if (m_FreeIndexList.empty())
        {
            return -1;
        }

        ++m_RegisteredThreadNum;

        nThreadIndex = m_FreeIndexList.front();
        m_FreeIndexList.pop_front();
    }

    m_ThreadArray[nThreadIndex].m_ulThreadID   = as_get_threadid();
    m_ThreadArray[nThreadIndex].m_nThreadIndex = nThreadIndex;
    m_ThreadArray[nThreadIndex].m_ulProcessNum = 0;
    (void)strncpy(m_ThreadArray[nThreadIndex].m_szThreadName,
                  pszThreadName,
                  MAX_THREAD_NAME);
    m_ThreadArray[nThreadIndex].m_tvStartTime  = time(NULL);
    m_ThreadArray[nThreadIndex].m_tvAliveTime  = m_ThreadArray[nThreadIndex].m_tvStartTime;
    m_ThreadArray[nThreadIndex].m_pReporter    = pReporter;

    AS_LOG(AS_LOG_INFO, "[deamon thread]regist thread[%s:%u], index[%d],pReporter[0x%08x]",
              m_ThreadArray[nThreadIndex].m_szThreadName,
              m_ThreadArray[nThreadIndex].m_ulThreadID,
              m_ThreadArray[nThreadIndex].m_nThreadIndex,
              pReporter);
    return nThreadIndex;
}


int32_t as_task_manage::UnregistThread(int32_t nThreadIndex)
{
    if ((0 > nThreadIndex)
         || (MAX_THREAD_NUM <= nThreadIndex))
    {
        return -1;
    }


    int32_t nIndex = m_ThreadArray[nThreadIndex].m_nThreadIndex;
    if (nIndex != nThreadIndex)
    {
        return -1;
    }

    AS_LOG(AS_LOG_INFO, "[deamon thread]unregist thread[%s:%u], index[%d]",
              m_ThreadArray[nThreadIndex].m_szThreadName,
              m_ThreadArray[nThreadIndex].m_ulThreadID,
              m_ThreadArray[nThreadIndex].m_nThreadIndex);


    m_ThreadArray[nThreadIndex].m_ulThreadID   = 0;
    m_ThreadArray[nThreadIndex].m_nThreadIndex = 0;
    m_ThreadArray[nThreadIndex].m_ulProcessNum = 0;
    memset(m_ThreadArray[nThreadIndex].m_szThreadName, 0x0, MAX_THREAD_NAME + 1);
    m_ThreadArray[nThreadIndex].m_tvStartTime  = time(NULL);
    m_ThreadArray[nThreadIndex].m_tvAliveTime  = time(NULL);
    m_ThreadArray[nThreadIndex].m_pReporter = NULL;


    as_lock_guard locker(m_FreeIndexListMutex);
    m_FreeIndexList.push_back(nThreadIndex);

    --m_RegisteredThreadNum;
    return 0;
}


int32_t as_task_manage::open()
{
    m_nThreadFlag              = true;
    /* create the deamon thread */
    size_t stack_size = AS_THREAD_STACK_DEFAULT;
    int32_t nRet = as_create_thread(deamon_monitor_thread,this,&m_threadHandle,stack_size);
    if (AS_ERROR_CODE_OK != nRet)
    {
        AS_LOG(AS_LOG_ERROR, "[deamon thread]open deamon thread fail.");
    }

    return nRet;
}


#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
 void* as_task_manage::deamon_monitor_thread(void *arg)
#elif AS_APP_OS == AS_OS_WIN32
 uint32_t __stdcall as_task_manage::deamon_monitor_thread(void *arg)
#endif
{
    as_task_manage* pMgr = (as_task_manage*)arg;
    pMgr->svc();
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    return NULL;
#elif AS_APP_OS == AS_OS_WIN32
    return 0;
#endif   
}

void as_task_manage::svc(void)
{
    uint32_t threadID = as_get_threadid();
    AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread[%u] run.", threadID);

    time_t curtime;
    ThreadInfo *pThreadInfo = NULL;
    m_tvLastCheckTime       = time(NULL);

    while (m_nThreadFlag)
    {
        for (int32_t i = 0; i < DEFAULT_THREAD_CHECK_INTERVAL; i++)
        {
             as_sleep(1000);
             if (!m_nThreadFlag)
             {
                 return ;
             }
        }

        curtime = time(NULL);
        if ((curtime < m_tvLastCheckTime)
            || (curtime - m_tvLastCheckTime > m_ulMaxThreadCheckInterval))
        {
            AS_LOG(AS_LOG_WARNING, "[deamon thread]systime is abnormal.");
            m_tvLastCheckTime = curtime;
            continue;
        }

        AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread begin");

        for (int32_t nIndex = 0; nIndex < MAX_THREAD_NUM; nIndex++)
        {
            pThreadInfo = &m_ThreadArray[nIndex];

            if ((0 == pThreadInfo->m_ulThreadID) || (NULL == pThreadInfo->m_pReporter))
            {
                continue;
            }

            time_t lastReportTime = pThreadInfo->m_pReporter->getLastReportTime();
            AS_LOG(AS_LOG_INFO,
                "[deamon thread]Check thread active status."
                "thread[%s:%u], index[%d],last report at last check period, thread last report[%d].",
                pThreadInfo->m_szThreadName,
                pThreadInfo->m_ulThreadID,
                pThreadInfo->m_nThreadIndex,
                pThreadInfo->m_tvAliveTime);

            pThreadInfo->m_tvAliveTime = lastReportTime;
            pThreadInfo->m_ulProcessNum = pThreadInfo->m_pReporter->getProcessNum();
            if (curtime - pThreadInfo->m_tvAliveTime > DEFAULT_THREAD_CHECK_INTERVAL)
            {
                AS_LOG(AS_LOG_WARNING, "[deamon thread]thread[%s:%u], index[%d] has"
                               " not update stat 60s, last report[%d].",
                               pThreadInfo->m_szThreadName,
                               pThreadInfo->m_ulThreadID,
                               pThreadInfo->m_nThreadIndex,
                               pThreadInfo->m_tvAliveTime);
            }

            if (curtime - pThreadInfo->m_tvAliveTime > m_ulMaxThreadCheckInterval)
            {
                if (0 != pThreadInfo->m_tvAliveTime)
                {
                    AS_LOG(AS_LOG_CRITICAL, "[deamon thread]thread[%s:%u], index[%d] has"
                               " not update stat too int32_t time, last report[%d],"
                               "restart flag[%d] dump flag[%d].",
                               pThreadInfo->m_szThreadName,
                               pThreadInfo->m_ulThreadID,
                               pThreadInfo->m_nThreadIndex,
                               pThreadInfo->m_tvAliveTime,
                               m_ulRestartServer,
                               m_ulCoreDump);
#if AS_APP_OS == AS_OS_LINUX
                    (void)system("dmesg -c");
                    (void)system("echo \"1\" >/proc/sys/kernel/sysrq");
                    (void)system("echo \"m\" >/proc/sysrq-trigger");
                    (void)system("dmesg");

                    if (1 <= m_ulRestartServer)
                    {
                        if (1 <= m_ulCoreDump)
                        {
                            (void)kill(getpid(), SIGABRT);
                        }
                        else
                        {
                            (void)kill(getpid(), SIGKILL);
                        }
                    }
#endif
                }
            }
        }

        AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread end");
        m_tvLastCheckTime = curtime;
    }

    AS_LOG(AS_LOG_INFO, "[deamon thread]deamon thread[%u] exit.", threadID);
    return ;
}


void as_task_manage::close()
{
    m_nThreadFlag = false;
    as_sleep(1000);
    AS_LOG(AS_LOG_INFO, "[deamon thread]close deamon thread");
    return ;
}

as_thread_reporter::as_thread_reporter()
{
    m_ulProcessNum       = 0;
    m_nThreadIndex       = -1;
    m_tvLastReportTime   = time(NULL);
}


as_thread_reporter::as_thread_reporter(const char *pszThreadName)
{
    m_ulProcessNum       = 0;
    m_tvLastReportTime   = time(NULL);

    m_nThreadIndex       = as_task_manage::instance()->RegistThread(this,pszThreadName);
    AS_LOG(AS_LOG_INFO, "as_thread_reporter::as_thread_reporter(), index[%d]",
                  m_nThreadIndex);
}

as_thread_reporter::~as_thread_reporter()
{
    try
    {
        (void)as_task_manage::instance()->UnregistThread(m_nThreadIndex);
        AS_LOG(AS_LOG_INFO, "as_thread_reporter::~as_thread_reporter(), index[%d]",
                  m_nThreadIndex);
    }
    catch(...)
    {
    }
}


void as_thread_reporter::ReportStat(uint32_t ulProcessNum)
{
    time_t curtime = time(NULL);
    m_tvLastReportTime = curtime;
    m_ulProcessNum += ulProcessNum ;
}


time_t as_thread_reporter::getLastReportTime()const
{
    return m_tvLastReportTime;
}


uint32_t as_thread_reporter::getProcessNum()const
{
    return m_ulProcessNum;
}
