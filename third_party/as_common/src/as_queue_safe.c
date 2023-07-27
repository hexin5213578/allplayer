#include "as_thread.h"
#include "as_config.h"
#include "as_common.h"
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <pthread.h>
#elif AS_APP_OS == AS_OS_WIN32
#endif
#include "as_queue_safe.h"

typedef struct as_safe_node_s {
    as_queue_t   queue;
    void        *data;
}as_safe_node_t;

as_safe_queue_t* as_safe_queue_create()
{
    as_safe_queue_t* s_queue = NULL;
    do {
        s_queue = (as_safe_queue_t *)(void*) malloc(sizeof(as_safe_queue_t));
        if (NULL == s_queue) {
            break;
        }
        as_queue_init(&s_queue->head);
        s_queue->lock = as_create_mutex();
        if (NULL == s_queue->lock) {
            break;
        }
        s_queue->size = 0;
        return s_queue;
    }while (0);

    if(NULL != s_queue) {
        if(NULL != s_queue->lock) {
            as_destroy_mutex(s_queue->lock);
            s_queue->lock = NULL;
        }
        free( s_queue );
        s_queue = NULL;
    }
    return s_queue;
}
void    as_safe_queue_destory(as_safe_queue_t* s_queue)
{
    if(NULL != s_queue) {
        if(NULL != s_queue->lock) {
            as_destroy_mutex(s_queue->lock);
            s_queue->lock = NULL;
        }
        free( s_queue );
    }
    return;
}
int32_t as_safe_queue_push_back(as_safe_queue_t* s_queue,void* data)
{
    as_safe_node_t* n = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(s_queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }

    do {
        n = (as_safe_node_t *)(void*) malloc(sizeof(as_safe_node_t));
        if (NULL == n) {
            break;
        }
        n->data = data;
        n->queue.next = NULL;
        n->queue.prev = NULL;
        as_queue_insert_tail(&s_queue->head,&n->queue);
        s_queue->size++;
        nRet = AS_ERROR_CODE_OK;
    }while (0);

    as_mutex_unlock(s_queue->lock);
    return nRet;
}
int32_t as_safe_queue_pop_back(as_safe_queue_t* s_queue,void** data)
{
    as_safe_node_t* n = NULL;
    as_queue_t*     q = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(s_queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }
    do{
        if(0 == s_queue->size) {
            break;
        }
        q = as_queue_last(&s_queue->head);
        as_queue_remove(q);
        n = (as_safe_node_t*)q;
        s_queue->size--;

        *data = n->data;

        free(n);
        nRet = AS_ERROR_CODE_OK;
    }while (0);    
    
    as_mutex_unlock(s_queue->lock);

    return nRet;
}
int32_t as_safe_queue_push_front(as_safe_queue_t* s_queue,void* data)
{
    as_safe_node_t* n = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(s_queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }

    do {
        n = (as_safe_node_t *)(void*) malloc(sizeof(as_safe_node_t));
        if (NULL == n) {
          
            break;
        }
        n->queue.next = NULL;
        n->queue.prev = NULL;
        n->data = data;
        as_queue_insert_head(&s_queue->head,&n->queue);
        s_queue->size++;
        nRet = AS_ERROR_CODE_OK;
    }while (0);

    as_mutex_unlock(s_queue->lock);
    return nRet;
}
int32_t as_safe_queue_pop_front(as_safe_queue_t* s_queue,void** data)
{
    as_safe_node_t* n = NULL;
    as_queue_t*     q = NULL;
    int32_t nRet = AS_ERROR_CODE_FAIL;
    if(AS_ERROR_CODE_OK != as_mutex_lock(s_queue->lock)) {
        return AS_ERROR_CODE_FAIL;
    }
    do{
        if(0 == s_queue->size) {
            break;
        }
        q = as_queue_head(&s_queue->head);
        as_queue_remove(q);
        n = (as_safe_node_t*)q;
        s_queue->size--;
        *data = n->data;
        free(n);
        nRet = AS_ERROR_CODE_OK;
    }while (0);    
    
    as_mutex_unlock(s_queue->lock);

    return nRet;
}

uint32_t as_safe_queue_size(as_safe_queue_t* s_queue)
{
    uint32_t size = 0;
    if(AS_ERROR_CODE_OK != as_mutex_lock(s_queue->lock)) {
        return size;
    }
    if(as_queue_empty(&s_queue->head)) {
        size = 0;
    }
    else {
        size = s_queue->size;
    }   
    
    as_mutex_unlock(s_queue->lock);

    return size;
}