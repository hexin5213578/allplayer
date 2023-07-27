#ifndef __AS_URL_H_INCLUDE
#define __AS_URL_H_INCLUDE

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include "as_common.h"
#include <stdint.h>
#include <stdio.h>
#include <signal.h>

#define AS_UNESCAPE_URI       1
#define AS_UNESCAPE_REDIRECT  2

#define AS_ULR_PROTOCOL_LEN  8
#define AS_URL_MAX_LEN       512
#define AS_URL_ARG_NAME_LEN  64
#define AS_URL_ARG_VALUE_LEN 512

typedef struct tagASUrl
{
    char     protocol[AS_ULR_PROTOCOL_LEN + 1];
    char     username[AS_URL_ARG_NAME_LEN + 1];
    char     password[AS_URL_ARG_NAME_LEN + 1];
    char     host[AS_URL_MAX_LEN + 1];
    uint16_t port;
    char     path[AS_URL_MAX_LEN + 1];
    char     uri[AS_URL_MAX_LEN + 1];
    char     args[AS_URL_MAX_LEN + 1];
}as_url_t;

typedef struct tagASUrlArg
{
    char     name[AS_URL_ARG_NAME_LEN + 1];
    char     value[AS_URL_ARG_VALUE_LEN + 1];
    char*    next;
}as_url_arg_t;

void       as_init_url(as_url_t* url);
int32_t    as_parse_url(const char* url,as_url_t* info);
int32_t    as_first_arg(as_url_t* url,as_url_arg_t* arg);
int32_t    as_next_arg(as_url_arg_t* arg,as_url_arg_t* next);
int32_t    as_find_arg(as_url_t* url,const char* name,as_url_arg_t* arg);

void       as_unescape_uri(uint8_t **dst, uint8_t **src, size_t size, uint32_t type);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#endif // __AS_URL_H_INCLUDE