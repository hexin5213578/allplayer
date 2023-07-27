

#include "as.h"
#include "as_synchronized.h"
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#endif

#if AS_APP_OS == AS_OS_WIN32
int32_t as_synchronized::start()
{
    semEvent = CreateEvent(0, FALSE, FALSE, 0);
    if( AS_NULL == semEvent )
    {
        return AS_ERROR_CODE_SYS;
    }
    semMutex = CreateMutex(0, FALSE, 0);
    if( AS_NULL == semMutex )
    {
        (void)CloseHandle(semEvent);
        return AS_ERROR_CODE_SYS;
    }    

    semPushEvent = CreateEvent(0, FALSE, FALSE, 0);
    if( AS_NULL == semPushEvent )    
    {
        (void)CloseHandle(semEvent);
        (void)CloseHandle(semMutex);
        return AS_ERROR_CODE_SYS;        
    }    
    
    semPushMutex = CreateMutex(0, FALSE, 0);
    if( AS_NULL == semPushMutex )    
    {
        (void)CloseHandle(semEvent);
        (void)CloseHandle(semMutex);
        (void)CloseHandle(semPushEvent);
        return AS_ERROR_CODE_SYS;        
    }    

    return AS_ERROR_CODE_OK ;
}
#else
int32_t as_synchronized::start()
{
    int32_t result = AS_ERROR_CODE_OK ;
    
    memset(&monitor, 0, sizeof(monitor));
    result = pthread_mutex_init(&monitor, 0);
    if( AS_ERROR_CODE_OK != result )
    {
        return result;
    }

    memset(&push_monitor, 0, sizeof(push_monitor));
    result = pthread_mutex_init(&push_monitor, 0);
    if( AS_ERROR_CODE_OK != result )
    {
        pthread_mutex_destroy(&monitor);
        return result;
    }    

    memset(&pop_cond, 0, sizeof(pop_cond));
    result = pthread_cond_init(&pop_cond, 0);
    if( AS_ERROR_CODE_OK != result )
    {
        pthread_mutex_destroy(&monitor);
        pthread_mutex_destroy(&push_monitor);        
        return result;
    }
    
    memset(&push_cond, 0, sizeof(push_cond));
    result = pthread_cond_init(&push_cond, 0);
    if( AS_ERROR_CODE_OK != result )
    {
        pthread_cond_destroy(&pop_cond);
        pthread_mutex_destroy(&monitor);
        pthread_mutex_destroy(&push_monitor);        
        return result;
    }
    
    return result ;
}
#endif

as_synchronized::as_synchronized()
{
#if AS_APP_OS == AS_OS_WIN32
    numNotifies = 0;
    semEvent = NULL;
    semMutex = NULL;
    semPushEvent = NULL;
    semPushMutex = NULL;    
#else     
#endif
}

as_synchronized::~as_synchronized()
{
#if AS_APP_OS == AS_OS_WIN32    
    (void)CloseHandle(semEvent);
    semEvent = NULL;
    (void)CloseHandle(semMutex);
    semMutex = NULL;

    (void)CloseHandle(semPushEvent);
    semPushEvent = NULL;
    (void)CloseHandle(semPushMutex);
    semPushMutex = NULL;

#else    
    pthread_cond_destroy(&push_cond);
    pthread_cond_destroy(&pop_cond);
    pthread_mutex_destroy(&monitor);
    pthread_mutex_destroy(&push_monitor);
#endif    
}

int32_t as_synchronized::popWait( int32_t timeout )
{
    int32_t result;
#if AS_APP_OS == AS_OS_WIN32
    result = wait(semEvent,semMutex,timeout);
#else
    result = wait(&pop_cond,&monitor,timeout);
#endif
    return result;
}

int32_t as_synchronized::pushWait( int32_t timeout )
{
    int32_t result;
#if AS_APP_OS == AS_OS_WIN32
    result = wait(semPushEvent,semMutex,timeout);
#else
    result = wait(&push_cond,&monitor,timeout);
#endif
    return result;
}

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
int32_t as_synchronized::cond_timed_wait( pthread_cond_t *cond,pthread_mutex_t *monitor,struct timespec *ts) 
{
    int32_t result;

    if(ts) 
    {
        result = pthread_cond_timedwait(cond, monitor, ts);
    }
    else 
    {
        result = pthread_cond_wait(cond, monitor);
    }

    return result;
}
#endif


#if AS_APP_OS == AS_OS_WIN32
int32_t  as_synchronized::wait(HANDLE hSemEvent,HANDLE hSemMutex,int32_t timeout)const
{
    uint32_t err;
    int32_t result = AS_ERROR_CODE_OK ;           

    if(!ReleaseMutex(hSemMutex))
    {
        return AS_ERROR_CODE_SYS;
    }

    if( timeout == 0 ) 
    {
        timeout = (int32_t)INFINITE ;
	}       

    err = WaitForSingleObject(hSemEvent, (uint32_t)timeout);
    switch(err)
    {
        case WAIT_TIMEOUT:
            result = AS_ERROR_CODE_TIMEOUT;
            break;
        case WAIT_ABANDONED:
            result = AS_ERROR_CODE_SYS;
            break;
        case WAIT_OBJECT_0:
            result = AS_ERROR_CODE_OK;
            break;
        default:     
            result = AS_ERROR_CODE_SYS;                
            break;                        
    }

    if(WaitForSingleObject (hSemMutex, INFINITE) != WAIT_OBJECT_0)
    {
        return AS_ERROR_CODE_SYS;
    } 

    return result ;
}        
#else
int32_t  as_synchronized::wait(pthread_cond_t *cond,pthread_mutex_t *monitor,int32_t timeout)
{
    struct timespec ts;
    struct timeval  tv;

    int32_t result= AS_ERROR_CODE_OK ;

    gettimeofday(&tv, 0);
    ts.tv_sec  = tv.tv_sec  + (int32_t)timeout/1000;
    ts.tv_nsec = (tv.tv_usec + (timeout %1000)*1000) * 1000; 

    int32_t err;
    if( timeout )
    {
        err = cond_timed_wait(cond,monitor,&ts);
    }
    else
    {
        err = cond_timed_wait(cond,monitor,AS_NULL);
    }
    if( err  > AS_ERROR_CODE_OK )
    {
        switch(err)
        {
            case ETIMEDOUT:
                 result = AS_ERROR_CODE_TIMEOUT;
            break;

            default:
                 result = AS_ERROR_CODE_SYS;
            break;
        }
    }

    return result;
}
#endif

#if AS_APP_OS == AS_OS_WIN32
int32_t  as_synchronized::notifyRead()
{
    numNotifies = 1;
    if(!SetEvent(semEvent))
    {
        return AS_ERROR_CODE_SYS;
    }

    return AS_ERROR_CODE_OK ;
}
#else
int32_t  as_synchronized::notifyRead()
{
    int32_t result;

    result = pthread_cond_signal(&pop_cond);
    if(result)
    {
        return result; 
    }

    return AS_ERROR_CODE_OK ;
}
#endif

#if AS_APP_OS == AS_OS_WIN32
int32_t  as_synchronized::notifyWrite()
{
    numNotifies = 1;
    if(!SetEvent(semPushEvent))
    {
        return AS_ERROR_CODE_SYS;
    }

    return AS_ERROR_CODE_OK ;
}
#else
int32_t  as_synchronized::notifyWrite()
{
    int32_t result;

    result = pthread_cond_signal(&push_cond);
    if(result)
    {
        return result; 
    }

    return AS_ERROR_CODE_OK ;
}
#endif

#if AS_APP_OS == AS_OS_WIN32
int32_t as_synchronized::notify_all()
{
    numNotifies = (char)0x80;
    while (numNotifies--)
    {
        if(!SetEvent(semEvent))
        {
            return AS_ERROR_CODE_SYS;
        }
    }

    return AS_ERROR_CODE_OK;            
}
#else
int32_t as_synchronized::notify_all()
{
    int32_t result;

    result = pthread_cond_broadcast(&pop_cond);
    if(result)
    {
        return result ;
    }

    return AS_ERROR_CODE_OK;    
}
#endif

#if AS_APP_OS == AS_OS_WIN32
int32_t as_synchronized::lock()
{
    if(WaitForSingleObject(semMutex, INFINITE) != WAIT_OBJECT_0)
    {
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}    
#else
int32_t as_synchronized::lock()
{
    if(pthread_mutex_lock(&monitor))
    {
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK ;    
}
#endif



#if AS_APP_OS == AS_OS_WIN32
int32_t as_synchronized::unlock()/*lint !e1714*/ //���ⲿ����
{
    if(!ReleaseMutex(semMutex))
    {
        return AS_ERROR_CODE_FAIL ;
    }

    return AS_ERROR_CODE_OK ;
}
#else
int32_t as_synchronized::unlock()
{
    if(pthread_mutex_unlock(&monitor))
    {
        return AS_ERROR_CODE_FAIL ;
    }

    return AS_ERROR_CODE_OK ;
}
#endif

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
AS_BOOLEAN as_synchronized::trylock()
{
    int32_t result = AS_ERROR_CODE_OK;

    result = pthread_mutex_trylock(&monitor);
    if( AS_ERROR_CODE_OK == result )
    {
        return AS_TRUE ;
    }
    else
    {
        return AS_FALSE ;
    }
}
#endif




