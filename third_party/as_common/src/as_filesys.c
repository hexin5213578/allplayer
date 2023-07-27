#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */
#include "as_config.h"
#include "as_basetype.h"
#include "as_common.h"
#include "as_filesys.h"
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

int32_t as_mkdir_full(unsigned char *dir, uint32_t access)
{
    unsigned char     *p, ch;
    int32_t      err;

    err = 0;

#if (WIN32)
    p = dir + 3;
#else
    p = dir + 1;
#endif

    for ( /* void */ ; *p; p++) {
        ch = *p;

        if (ch != '/') {
            continue;
        }

        *p = '\0';
        if (mkdir((const char *)dir, access) == -1) {
            err = errno;

            switch (err) {
            case EEXIST:
                err = 0;
                break;
            case EACCES:
                break;
            default:
                return err;
            }
        }

        *p = '/';
    }

    if (mkdir((const char *)dir, access) == -1) {
        err = errno;

        switch (err) {
        case EEXIST:
            err = 0;
            break;
        case EACCES:
            break;
        default:
            return err;
        }
    }

    return err;
}

AS_BOOLEAN as_is_directory(const char *strDir) 
{
#if AS_APP_OS == AS_OS_WIN32
    struct _stat sstat;
    if(0 != _stat(strDir, &sstat))
    {
        return AS_FALSE;
    }
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    struct stat sstat;
    if(0 != stat(strDir, &sstat))
    {
        return AS_FALSE;
    }
#endif
    if(0 != (S_IFDIR & sstat.st_mode))
    {
        return AS_TRUE;
    }
    return AS_FALSE;
}

int32_t as_remove_dir(const char *dir)
{
    char strBuffer[AS_MAX_FILE_PATH_LEN];
    char szCurPath[AS_MAX_FILE_PATH_LEN];
#if AS_APP_OS == AS_OS_WIN32
    _snprintf(szCurPath, AS_MAX_FILE_PATH_LEN, "%s//*.*", dir);
    WIN32_FIND_DATAA FindFileData;
    ZeroMemory(&FindFileData, sizeof(WIN32_FIND_DATAA));
    HANDLE hFile = FindFirstFileA(szCurPath, &FindFileData);
    BOOL IsFinded = TRUE;
    while (IsFinded)
    {
        IsFinded = FindNextFileA(hFile, &FindFileData);
        if (strcmp(FindFileData.cFileName, ".") && strcmp(FindFileData.cFileName, "..")) 
        {
            _snprintf(strBuffer, AS_MAX_FILE_PATH_LEN, "%s//%s", dir,FindFileData.cFileName); 
            if (AS_TRUE == as_is_directory(strBuffer))
            {
                as_remove_dir(strBuffer);
            }
            else
            {
                DeleteFileA(strBuffer);
            }
        }
    }
    FindClose(hFile);

    RemoveDirectoryA(dir);

#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    DIR* pDir = NULL;
    struct dirent *pDirent = NULL;

    if (NULL == (pDir = opendir(dir)))
    {
        return -1;
    }

    while (NULL != (pDirent = readdir(pDir)))
    {
        if ( (0 == strcmp(pDirent->d_name, ".")) || (0 == strcmp(pDirent->d_name, "..")))
        {
            continue;
        }

        (void)snprintf(strBuffer, sizeof(strBuffer),
                           "%s/%s",
                           dir,
                           pDirent->d_name);
        if(AS_FALSE == as_is_directory(strBuffer))
        {
            (void)unlink(strBuffer);
            continue;
        }
        
        (void)as_remove_dir(strBuffer);
    }
    (void)closedir(pDir);
    rmdir(dir);
#endif
    return AS_ERROR_CODE_OK;
}

int32_t as_walk_tree(as_tree_ctx_t *ctx, const char *tree)
{
    char            *name;
    size_t           len;
    int              rc;
    int32_t          err;    
    char             path[AS_MAX_FILE_PATH_LEN] = {0};
    char             szCurPath[AS_MAX_FILE_PATH_LEN];

    rc = AS_ERROR_CODE_OK;

 #if AS_APP_OS == AS_OS_WIN32
    BOOL IsFinded = TRUE;
    struct _stat info;
    _snprintf(szCurPath, AS_MAX_FILE_PATH_LEN, "%s//*.*", tree);
    WIN32_FIND_DATAA FindFileData;
    ZeroMemory(&FindFileData, sizeof(WIN32_FIND_DATAA));
    HANDLE hFile = FindFirstFileA(szCurPath, &FindFileData);
 #elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX   
    DIR             *dir;
    struct dirent   *de;
    struct stat      info;
    dir = opendir(tree);
    if (NULL == dir) {
        return AS_ERROR_CODE_FAIL;
    }
#endif
    for ( ;; ) {
#if AS_APP_OS == AS_OS_WIN32
    if(IsFinded) {
        goto done;
    }
    IsFinded = FindNextFileA(hFile, &FindFileData);    
    name = (char*)&FindFileData.cFileName[0];
    len = strlen(name);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
        errno = 0;
        de = readdir(dir);

        if (NULL == de) {
            err = errno;

            if (err == 0) {
                rc = AS_ERROR_CODE_OK;
            } else {
                rc = AS_ERROR_CODE_FAIL;
            }
            goto done;
        }

        len = strlen(de->d_name);
        name = (char*)de->d_name;
#endif
        if (len == 1 && name[0] == '.') {
            continue;
        }

        if (len == 2 && name[0] == '.' && name[1] == '.') {
            continue;
        }

        snprintf(path,AS_MAX_FILE_PATH_LEN,"%s/%s",tree,name);
#if AS_APP_OS == AS_OS_WIN32
        if(0 != _stat(path, &info))
        {
            return AS_FALSE;
        }
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
        if( -1 == stat((const char *)&path[0],&info)) {
            return AS_ERROR_CODE_FAIL;
        }
#endif
        if ((S_ISREG(info.st_mode)) && (NULL != ctx->file_handler)) {

            ctx->size    = info.st_size;
#if AS_APP_OS == AS_OS_WIN32
            ctx->fs_size = info.st_size;
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
            ctx->fs_size = (info.st_size < info.st_blocks * 512) ? (info.st_blocks * 512) : (info.st_size);
#endif
            ctx->access  = info.st_mode & 0777;
            ctx->mtime   = info.st_mtime;

            if (ctx->file_handler(ctx, (char *)&path[0]) == AS_ERROR_CODE_ABORT) {
                goto failed;
            }

        } else if (S_ISDIR(info.st_mode)) {

            ctx->access = info.st_mode & 0777;
            ctx->mtime  = info.st_mtime;

            if(NULL != ctx->pre_tree_handler) {
                rc = ctx->pre_tree_handler(ctx,path);

                if (rc == AS_ERROR_CODE_ABORT) {
                    goto failed;
                }

                if (rc == AS_ERROR_CODE_DECLINED) {
                    continue;
                }
            }

            if (as_walk_tree(ctx, (char*)&path[0]) == AS_ERROR_CODE_ABORT) {
                goto failed;
            }

            ctx->access = info.st_mode & 0777;
            ctx->mtime  = info.st_mtime;
            if(NULL != ctx->post_tree_handler) {
                if (ctx->post_tree_handler(ctx, (char*)&path[0]) == AS_ERROR_CODE_ABORT) {
                    goto failed;
                }
            }

        } else {
            if(NULL != ctx->post_tree_handler) {
                if (ctx->spec_handler(ctx, (char*)&path[0]) == AS_ERROR_CODE_ABORT) {
                    goto failed;
                }
            }
        }
    }

failed:
    rc = AS_ERROR_CODE_ABORT;
done:
#if AS_APP_OS == AS_OS_WIN32
    return rc;
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX  
    closedir(dir);
    dir = NULL;
    return rc;
#endif
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

