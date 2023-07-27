#ifndef __AS_TIMER_H_INCLUDE
#define __AS_TIMER_H_INCLUDE

#ifdef WIN32
#pragma warning(disable: 4786)
#endif

#include <list>
#include <map>
extern "C"{
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
}

const ULONG DefaultTimerScale = 100; 
const ULONG MinTimerScale = 1;

class ITrigger;
class CTimerItem;

class CCmpTimeOut
{
  public:
    bool operator()(const ULONGLONG ullTimerOut1, const ULONGLONG ullTimerOut2) const
    {
        return (ullTimerOut1 < ullTimerOut2);
    };
};
typedef std::multimap<ULONGLONG, CTimerItem *, CCmpTimeOut> ListOfTrigger;
typedef std::pair<ULONGLONG const, CTimerItem *> ListOfTriggerPair;
typedef ListOfTrigger::iterator ListOfTriggerIte;

typedef enum tagTriggerStyle
{
    enOneShot = 0,
    enRepeated = 1
} TriggerStyle;

class ITrigger
{
  public:
    ITrigger()
    {
        m_pTimerItem = NULL;
    };
    virtual ~ITrigger(){};

  public:
    virtual void onTrigger(void *pArg, ULONGLONG ullScales, TriggerStyle enStyle) = 0;

  public:
    CTimerItem *m_pTimerItem;
};

class CTimerItem
{
  public:
    CTimerItem()
    {
        m_pTrigger = NULL;
        m_pArg = NULL;
        m_bRemoved = AS_FALSE;
    };

  public:
    ITrigger *m_pTrigger;
    void *m_pArg;
    ULONG m_ulInitialScales;
    ULONGLONG m_ullCurScales;
    TriggerStyle m_enStyle;
    AS_BOOLEAN m_bRemoved;
};

#define    TIMER_OPERATOR_LOG    16
#define    TIMER_RUN_LOG         17
#define    TIMER_SECURITY_LOG    20
#define    TIMER_USER_LOG        19

enum TIMER_LOG_LEVEL
{
    TIMER_EMERGENCY = 0,
    TIMER_ERROR = 3,
    TIMER_WARNING = 4,
    TIMER_DEBUG = 7
};


class ITimerLog
{
public:
    virtual void writeLog(int32_t iType, int32_t ilevel,
        const char *szLogDetail, const int32_t iLogLen) = 0;
};

extern ITimerLog *g_pTimerLog;

class as_timer
{
public:
    as_timer();
    virtual ~as_timer();
public:
    virtual int32_t init(ULONG ulTimerScale);
    void setLogWriter(ITimerLog *pTimerLog)
    {
        g_pTimerLog = pTimerLog;
    };
    virtual int32_t run();
    void exit();

public:
     virtual int32_t registerTimer(ITrigger *pRrsTrigger, void *pArg, ULONG nScales,
        TriggerStyle enStyle);
     virtual int32_t cancelTimer(ITrigger *pRrsTrigger);

    void clearTimer( );
private:
    static void *invoke(void *argc)
    {
        as_timer *pTimer = (as_timer *)argc;
        pTimer->mainLoop();
        as_thread_exit(NULL);
        return NULL;
    };

    void mainLoop();

private:
    ULONG m_ulTimerScale;
    ULONGLONG m_ullRrsAbsTimeScales;
    ListOfTrigger *m_plistTrigger;
    as_mutex_t *m_pMutexListOfTrigger;
    as_thread_t *m_pASThread;
    volatile AS_BOOLEAN m_bExit;
};


#endif //__AS_TIMER_H_INCLUDE


