#ifndef __AS_SYNCHRONIZED_INCLUDE_H__
#define __AS_SYNCHRONIZED_INCLUDE_H__
extern "C"{
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
}
#if AS_APP_OS == AS_OS_WIN32
#include <windows.h>
#include <synchapi.h>
#endif

class as_synchronized
{
public:
    as_synchronized();
    virtual ~as_synchronized();

    int32_t   popWait(int32_t timeout);    
    int32_t   pushWait(int32_t timeout);    

    int32_t   start();

    int32_t   notifyRead();
    int32_t   notifyWrite();    
    int32_t   notify_all();
    int32_t  lock();
    AS_BOOLEAN  trylock();
    int32_t  unlock();

#if AS_APP_OS == AS_OS_WIN32
    int32_t     wait(HANDLE,HANDLE,int32_t)const;
    char    numNotifies;
    HANDLE  semEvent;
    HANDLE  semMutex;
    HANDLE  semPushEvent;
    HANDLE  semPushMutex;    
#else
    pthread_cond_t  pop_cond;
    pthread_cond_t  push_cond;    
    pthread_mutex_t monitor;
    pthread_mutex_t push_monitor;    
    int32_t  wait(pthread_cond_t *,pthread_mutex_t *,int32_t );
    int32_t  cond_timed_wait( pthread_cond_t * ,pthread_mutex_t *,timespec *);    
#endif
};

#endif /* __AS_SYNCHRONIZED_INCLUDE_H__ */
