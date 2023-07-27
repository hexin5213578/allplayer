#ifndef __AS_MEDIA_KENERL_SAFE_QUEUE_H__
#define __AS_MEDIA_KENERL_SAFE_QUEUE_H__
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
#include "as_mutex.h"
#include "as_queue.h"

typedef struct as_safe_queue_s {
    as_queue_t   head;
    as_mutex_t  *lock;
    uint32_t     size;
}as_safe_queue_t;


as_safe_queue_t* as_safe_queue_create();
void     as_safe_queue_destory(as_safe_queue_t* s_queue);
int32_t  as_safe_queue_push_back(as_safe_queue_t* s_queue,void* data);
int32_t  as_safe_queue_pop_back(as_safe_queue_t* s_queue,void** data);
int32_t  as_safe_queue_push_front(as_safe_queue_t* s_queue,void* data);
int32_t  as_safe_queue_pop_front(as_safe_queue_t* s_queue,void** data);
uint32_t as_safe_queue_size(as_safe_queue_t* s_queue);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* __AS_MEDIA_KENERL_SAFE_QUEUE_H__ */
