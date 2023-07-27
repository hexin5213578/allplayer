
#include "as_errno.h"
#include <cstdio>

#if defined(_WIN32)
#define FD_SETSIZE 1024 //修改默认64为1024路
#include <winsock2.h>
#include <windows.h>
#else
#include <cerrno>
#endif // defined(_WIN32)

static const char *as__unknown_err_code(int err) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "Unknown system error %d", err);
    return buf;
}

#define AS_ERR_NAME_GEN(name, _) case AS_ ## name: return #name;
const char *as_err_name(int err) {
    switch (err) {
        AS_ERRNO_MAP(AS_ERR_NAME_GEN)
    }
    return as__unknown_err_code(err);
}
#undef AS_ERR_NAME_GEN

#define AS_STRERROR_GEN(name, msg) case AS_ ## name: return msg;
const char *as_strerror(int err) {
    switch (err) {
        AS_ERRNO_MAP(AS_STRERROR_GEN)
    }
    return as__unknown_err_code(err);
}
#undef AS_STRERROR_GEN

int as_translate_posix_error(int err) {
    if (err <= 0) {
        return err;
    }
    switch (err) {
        //为了兼容windows/unix平台，信号EINPROGRESS ，EAGAIN，EWOULDBLOCK，ENOBUFS 全部统一成EAGAIN处理
        case ENOBUFS://在mac系统下实测发现会有此信号发生
        case EINPROGRESS:
        case EWOULDBLOCK: err = EAGAIN; break;
        default: break;
    }
    return -err;
}

int get_as_error(bool netErr) 
{
#if defined(_WIN32)
    auto errCode = netErr ? WSAGetLastError() : GetLastError();
    switch (errCode) 
    {
    case ERROR_NOACCESS:                    return AS_EACCES;
    case WSAEACCES:                         return AS_EACCES;
#if defined(ERROR_ELEVATION_REQUIRED)
    case ERROR_ELEVATION_REQUIRED:          return AS_EACCES;
#endif //ERROR_ELEVATION_REQUIRED
    case ERROR_ADDRESS_ALREADY_ASSOCIATED:  return AS_EADDRINUSE;
    case WSAEADDRINUSE:                     return AS_EADDRINUSE;
    case WSAEADDRNOTAVAIL:                  return AS_EADDRNOTAVAIL;
    case WSAEAFNOSUPPORT:                   return AS_EAFNOSUPPORT;
    case WSAEWOULDBLOCK:                    return AS_EAGAIN;
    case WSAEALREADY:                       return AS_EALREADY;
    case ERROR_INVALID_FLAGS:               return AS_EBADF;
    case ERROR_INVALID_HANDLE:              return AS_EBADF;
    case ERROR_LOCK_VIOLATION:              return AS_EBUSY;
    case ERROR_PIPE_BUSY:                   return AS_EBUSY;
    case ERROR_SHARING_VIOLATION:           return AS_EBUSY;
    case ERROR_OPERATION_ABORTED:           return AS_ECANCELED;
    case WSAEINTR:                          return AS_ECANCELED;
    case ERROR_NO_UNICODE_TRANSLATION:      return AS_ECHARSET;
    case ERROR_CONNECTION_ABORTED:          return AS_ECONNABORTED;
    case WSAECONNABORTED:                   return AS_ECONNABORTED;
    case ERROR_CONNECTION_REFUSED:          return AS_ECONNREFUSED;
    case WSAECONNREFUSED:                   return AS_ECONNREFUSED;
    case ERROR_NETNAME_DELETED:             return AS_ECONNRESET;
    case WSAECONNRESET:                     return AS_ECONNRESET;
    case ERROR_ALREADY_EXISTS:              return AS_EEXIST;
    case ERROR_FILE_EXISTS:                 return AS_EEXIST;
    case ERROR_BUFFER_OVERFLOW:             return AS_EFAULT;
    case WSAEFAULT:                         return AS_EFAULT;
    case ERROR_HOST_UNREACHABLE:            return AS_EHOSTUNREACH;
    case WSAEHOSTUNREACH:                   return AS_EHOSTUNREACH;
    case ERROR_INSUFFICIENT_BUFFER:         return AS_EINVAL;
    case ERROR_INVALID_DATA:                return AS_EINVAL;
    case ERROR_INVALID_PARAMETER:           return AS_EINVAL;
#if defined(ERROR_SYMLINK_NOT_SUPPORTED)
    case ERROR_SYMLINK_NOT_SUPPORTED:       return AS_EINVAL;
#endif //ERROR_SYMLINK_NOT_SUPPORTED
    case WSAEINVAL:                         return AS_EINVAL;
    case WSAEPFNOSUPPORT:                   return AS_EINVAL;
    case WSAESOCKTNOSUPPORT:                return AS_EINVAL;
    case ERROR_BEGINNING_OF_MEDIA:          return AS_EIO;
    case ERROR_BUS_RESET:                   return AS_EIO;
    case ERROR_CRC:                         return AS_EIO;
    case ERROR_DEVICE_DOOR_OPEN:            return AS_EIO;
    case ERROR_DEVICE_REQUIRES_CLEANING:    return AS_EIO;
    case ERROR_DISK_CORRUPT:                return AS_EIO;
    case ERROR_EOM_OVERFLOW:                return AS_EIO;
    case ERROR_FILEMARK_DETECTED:           return AS_EIO;
    case ERROR_GEN_FAILURE:                 return AS_EIO;
    case ERROR_INVALID_BLOCK_LENGTH:        return AS_EIO;
    case ERROR_IO_DEVICE:                   return AS_EIO;
    case ERROR_NO_DATA_DETECTED:            return AS_EIO;
    case ERROR_NO_SIGNAL_SENT:              return AS_EIO;
    case ERROR_OPEN_FAILED:                 return AS_EIO;
    case ERROR_SETMARK_DETECTED:            return AS_EIO;
    case ERROR_SIGNAL_REFUSED:              return AS_EIO;
    case WSAEISCONN:                        return AS_EISCONN;
    case ERROR_CANT_RESOLVE_FILENAME:       return AS_ELOOP;
    case ERROR_TOO_MANY_OPEN_FILES:         return AS_EMFILE;
    case WSAEMFILE:                         return AS_EMFILE;
    case WSAEMSGSIZE:                       return AS_EMSGSIZE;
    case ERROR_FILENAME_EXCED_RANGE:        return AS_ENAMETOOLONG;
    case ERROR_NETWORK_UNREACHABLE:         return AS_ENETUNREACH;
    case WSAENETUNREACH:                    return AS_ENETUNREACH;
    case WSAENOBUFS:                        return AS_ENOBUFS;
    case ERROR_BAD_PATHNAME:                return AS_ENOENT;
    case ERROR_DIRECTORY:                   return AS_ENOENT;
    case ERROR_FILE_NOT_FOUND:              return AS_ENOENT;
    case ERROR_INVALID_NAME:                return AS_ENOENT;
    case ERROR_INVALID_DRIVE:               return AS_ENOENT;
    case ERROR_INVALID_REPARSE_DATA:        return AS_ENOENT;
    case ERROR_MOD_NOT_FOUND:               return AS_ENOENT;
    case ERROR_PATH_NOT_FOUND:              return AS_ENOENT;
    case WSAHOST_NOT_FOUND:                 return AS_ENOENT;
    case WSANO_DATA:                        return AS_ENOENT;
    case ERROR_NOT_ENOUGH_MEMORY:           return AS_ENOMEM;
    case ERROR_OUTOFMEMORY:                 return AS_ENOMEM;
    case ERROR_CANNOT_MAKE:                 return AS_ENOSPC;
    case ERROR_DISK_FULL:                   return AS_ENOSPC;
    case ERROR_EA_TABLE_FULL:               return AS_ENOSPC;
    case ERROR_END_OF_MEDIA:                return AS_ENOSPC;
    case ERROR_HANDLE_DISK_FULL:            return AS_ENOSPC;
    case ERROR_NOT_CONNECTED:               return AS_ENOTCONN;
    case WSAENOTCONN:                       return AS_ENOTCONN;
    case ERROR_DIR_NOT_EMPTY:               return AS_ENOTEMPTY;
    case WSAENOTSOCK:                       return AS_ENOTSOCK;
    case ERROR_NOT_SUPPORTED:               return AS_ENOTSUP;
    case ERROR_BROKEN_PIPE:                 return AS_EOF;
    case ERROR_ACCESS_DENIED:               return AS_EPERM;
    case ERROR_PRIVILEGE_NOT_HELD:          return AS_EPERM;
    case ERROR_BAD_PIPE:                    return AS_EPIPE;
    case ERROR_NO_DATA:                     return AS_EPIPE;
    case ERROR_PIPE_NOT_CONNECTED:          return AS_EPIPE;
    case WSAESHUTDOWN:                      return AS_EPIPE;
    case WSAEPROTONOSUPPORT:                return AS_EPROTONOSUPPORT;
    case ERROR_WRITE_PROTECT:               return AS_EROFS;
    case ERROR_SEM_TIMEOUT:                 return AS_ETIMEDOUT;
    case WSAETIMEDOUT:                      return AS_ETIMEDOUT;
    case ERROR_NOT_SAME_DEVICE:             return AS_EXDEV;
    case ERROR_INVALID_FUNCTION:            return AS_EISDIR;
    case ERROR_META_EXPANSION_TOO_LONG:     return AS_E2BIG;
    default:                                return errCode;
    }
#else
    return as_translate_posix_error(errno);
#endif // defined(_WIN32)
}

const char *get_as_errmsg(bool netErr) {
    return as_strerror(get_as_error(netErr));
}