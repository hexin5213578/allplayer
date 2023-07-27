#ifndef __AS_STRING_H__
#define __AS_STRING_H__
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#include "as_config.h"
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#if AS_APP_OS == AS_OS_WIN32
#include <windows.h>
#endif
char *as_strsep(char **stringp, const char *delim);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif /* __AS_STRING_H__ */
