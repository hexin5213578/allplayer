#ifndef __AS_MEDIA_KENERL_THREAD_H__
#define __AS_MEDIA_KENERL_THREAD_H__
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#if AS_APP_OS == AS_OS_WIN32
#include <windows.h>
#endif

#define  AS_DEFAULT_STACK_SIZE (2*1024*1024)

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
typedef  void* ( * AS_THREAD_FUNC)(void *);
typedef struct tagASThread
{
    pthread_attr_t attr;
    pthread_t pthead;
}as_thread_t;

#elif AS_APP_OS == AS_OS_WIN32
typedef  uint32_t (__stdcall * AS_THREAD_FUNC)(void *);
typedef struct tagASThread
{
    uint32_t ptheadID;
    HANDLE pthead;
}as_thread_t;
#endif

int32_t  as_create_thread( AS_THREAD_FUNC pfnThread, void *args,
          as_thread_t **pstMKThread,uint32_t ulStackSize);
void     as_destory_thread(as_thread_t *pstMKThread);

int32_t  as_join_thread(as_thread_t *pstMKThread);
void     as_thread_exit(void *retval);
uint32_t as_get_threadid(void);
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
pthread_t  as_thread_self(void);
#elif AS_APP_OS == AS_OS_WIN32
HANDLE as_thread_self(void);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* __AS_MEDIA_KENERL_THREAD_H__ */
