#ifndef __AS_FILESYSTEM_H__
#define __AS_FILESYSTEM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */


#include "as_config.h"
#include "as_basetype.h"
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#if AS_APP_OS == AS_OS_WIN32
#include <windows.h>
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <pthread.h>
#include <dirent.h>
#endif
#include <sys/types.h>

#define AS_MAX_FILE_PATH_LEN 512

typedef struct as_tree_ctx_s  as_tree_ctx_t;

typedef int32_t (*as_tree_handler_pt) (as_tree_ctx_t *ctx, const char *name);

struct as_tree_ctx_s {
    off_t                      size;
    off_t                      fs_size;
    uint32_t                   access;
    time_t                     mtime;

    as_tree_handler_pt         file_handler;
    as_tree_handler_pt         pre_tree_handler;
    as_tree_handler_pt         post_tree_handler;
    as_tree_handler_pt         spec_handler;

    void                      *data;
};


int32_t    as_mkdir_full(unsigned char *dir, uint32_t access);

AS_BOOLEAN as_is_directory(const char *strDir) ;

int32_t    as_remove_dir(const char *dir);

int32_t    as_walk_tree(as_tree_ctx_t *ctx, const char *tree);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#endif

