/* 
*/

#ifndef AS_ERRNO_H_
#define AS_ERRNO_H_

#include <cerrno>

#define AS__EOF     (-4095)
#define AS__UNKNOWN (-4094)

#define AS__EAI_ADDRFAMILY  (-3000)
#define AS__EAI_AGAIN       (-3001)
#define AS__EAI_BADFLAGS    (-3002)
#define AS__EAI_CANCELED    (-3003)
#define AS__EAI_FAIL        (-3004)
#define AS__EAI_FAMILY      (-3005)
#define AS__EAI_MEMORY      (-3006)
#define AS__EAI_NODATA      (-3007)
#define AS__EAI_NONAME      (-3008)
#define AS__EAI_OVERFLOW    (-3009)
#define AS__EAI_SERVICE     (-3010)
#define AS__EAI_SOCKTYPE    (-3011)
#define AS__EAI_BADHINTS    (-3013)
#define AS__EAI_PROTOCOL    (-3014)

/* Only map to the system errno on non-Windows platforms. It's apparently
* a fairly common practice for Windows programmers to redefine errno codes.
*/
#if defined(E2BIG) && !defined(_WIN32)
# define AS__E2BIG (-E2BIG)
#else
# define AS__E2BIG (-4093)
#endif

#if defined(EACCES) && !defined(_WIN32)
# define AS__EACCES (-EACCES)
#else
# define AS__EACCES (-4092)
#endif

#if defined(EADDRINUSE) && !defined(_WIN32)
# define AS__EADDRINUSE (-EADDRINUSE)
#else
# define AS__EADDRINUSE (-4091)
#endif

#if defined(EADDRNOTAVAIL) && !defined(_WIN32)
# define AS__EADDRNOTAVAIL (-EADDRNOTAVAIL)
#else
# define AS__EADDRNOTAVAIL (-4090)
#endif

#if defined(EAFNOSUPPORT) && !defined(_WIN32)
# define AS__EAFNOSUPPORT (-EAFNOSUPPORT)
#else
# define AS__EAFNOSUPPORT (-4089)
#endif

#if defined(EAGAIN) && !defined(_WIN32)
# define AS__EAGAIN (-EAGAIN)
#else
# define AS__EAGAIN (-4088)
#endif

#if defined(EALREADY) && !defined(_WIN32)
# define AS__EALREADY (-EALREADY)
#else
# define AS__EALREADY (-4084)
#endif

#if defined(EBADF) && !defined(_WIN32)
# define AS__EBADF (-EBADF)
#else
# define AS__EBADF (-4083)
#endif

#if defined(EBUSY) && !defined(_WIN32)
# define AS__EBUSY (-EBUSY)
#else
# define AS__EBUSY (-4082)
#endif

#if defined(ECANCELED) && !defined(_WIN32)
# define AS__ECANCELED (-ECANCELED)
#else
# define AS__ECANCELED (-4081)
#endif

#if defined(ECHARSET) && !defined(_WIN32)
# define AS__ECHARSET (-ECHARSET)
#else
# define AS__ECHARSET (-4080)
#endif

#if defined(ECONNABORTED) && !defined(_WIN32)
# define AS__ECONNABORTED (-ECONNABORTED)
#else
# define AS__ECONNABORTED (-4079)
#endif

#if defined(ECONNREFUSED) && !defined(_WIN32)
# define AS__ECONNREFUSED (-ECONNREFUSED)
#else
# define AS__ECONNREFUSED (-4078)
#endif

#if defined(ECONNRESET) && !defined(_WIN32)
# define AS__ECONNRESET (-ECONNRESET)
#else
# define AS__ECONNRESET (-4077)
#endif

#if defined(EDESTADDRREQ) && !defined(_WIN32)
# define AS__EDESTADDRREQ (-EDESTADDRREQ)
#else
# define AS__EDESTADDRREQ (-4076)
#endif

#if defined(EEXIST) && !defined(_WIN32)
# define AS__EEXIST (-EEXIST)
#else
# define AS__EEXIST (-4075)
#endif

#if defined(EFAULT) && !defined(_WIN32)
# define AS__EFAULT (-EFAULT)
#else
# define AS__EFAULT (-4074)
#endif

#if defined(EHOSTUNREACH) && !defined(_WIN32)
# define AS__EHOSTUNREACH (-EHOSTUNREACH)
#else
# define AS__EHOSTUNREACH (-4073)
#endif

#if defined(EINTR) && !defined(_WIN32)
# define AS__EINTR (-EINTR)
#else
# define AS__EINTR (-4072)
#endif

#if defined(EINVAL) && !defined(_WIN32)
# define AS__EINVAL (-EINVAL)
#else
# define AS__EINVAL (-4071)
#endif

#if defined(EIO) && !defined(_WIN32)
# define AS__EIO (-EIO)
#else
# define AS__EIO (-4070)
#endif

#if defined(EISCONN) && !defined(_WIN32)
# define AS__EISCONN (-EISCONN)
#else
# define AS__EISCONN (-4069)
#endif

#if defined(EISDIR) && !defined(_WIN32)
# define AS__EISDIR (-EISDIR)
#else
# define AS__EISDIR (-4068)
#endif

#if defined(ELOOP) && !defined(_WIN32)
# define AS__ELOOP (-ELOOP)
#else
# define AS__ELOOP (-4067)
#endif

#if defined(EMFILE) && !defined(_WIN32)
# define AS__EMFILE (-EMFILE)
#else
# define AS__EMFILE (-4066)
#endif

#if defined(EMSGSIZE) && !defined(_WIN32)
# define AS__EMSGSIZE (-EMSGSIZE)
#else
# define AS__EMSGSIZE (-4065)
#endif

#if defined(ENAMETOOLONG) && !defined(_WIN32)
# define AS__ENAMETOOLONG (-ENAMETOOLONG)
#else
# define AS__ENAMETOOLONG (-4064)
#endif

#if defined(ENETDOWN) && !defined(_WIN32)
# define AS__ENETDOWN (-ENETDOWN)
#else
# define AS__ENETDOWN (-4063)
#endif

#if defined(ENETUNREACH) && !defined(_WIN32)
# define AS__ENETUNREACH (-ENETUNREACH)
#else
# define AS__ENETUNREACH (-4062)
#endif

#if defined(ENFILE) && !defined(_WIN32)
# define AS__ENFILE (-ENFILE)
#else
# define AS__ENFILE (-4061)
#endif

#if defined(ENOBUFS) && !defined(_WIN32)
# define AS__ENOBUFS (-ENOBUFS)
#else
# define AS__ENOBUFS (-4060)
#endif

#if defined(ENODEV) && !defined(_WIN32)
# define AS__ENODEV (-ENODEV)
#else
# define AS__ENODEV (-4059)
#endif

#if defined(ENOENT) && !defined(_WIN32)
# define AS__ENOENT (-ENOENT)
#else
# define AS__ENOENT (-4058)
#endif

#if defined(ENOMEM) && !defined(_WIN32)
# define AS__ENOMEM (-ENOMEM)
#else
# define AS__ENOMEM (-4057)
#endif

#if defined(ENONET) && !defined(_WIN32)
# define AS__ENONET (-ENONET)
#else
# define AS__ENONET (-4056)
#endif

#if defined(ENOSPC) && !defined(_WIN32)
# define AS__ENOSPC (-ENOSPC)
#else
# define AS__ENOSPC (-4055)
#endif

#if defined(ENOSYS) && !defined(_WIN32)
# define AS__ENOSYS (-ENOSYS)
#else
# define AS__ENOSYS (-4054)
#endif

#if defined(ENOTCONN) && !defined(_WIN32)
# define AS__ENOTCONN (-ENOTCONN)
#else
# define AS__ENOTCONN (-4053)
#endif

#if defined(ENOTDIR) && !defined(_WIN32)
# define AS__ENOTDIR (-ENOTDIR)
#else
# define AS__ENOTDIR (-4052)
#endif

#if defined(ENOTEMPTY) && !defined(_WIN32)
# define AS__ENOTEMPTY (-ENOTEMPTY)
#else
# define AS__ENOTEMPTY (-4051)
#endif

#if defined(ENOTSOCK) && !defined(_WIN32)
# define AS__ENOTSOCK (-ENOTSOCK)
#else
# define AS__ENOTSOCK (-4050)
#endif

#if defined(ENOTSUP) && !defined(_WIN32)
# define AS__ENOTSUP (-ENOTSUP)
#else
# define AS__ENOTSUP (-4049)
#endif

#if defined(EPERM) && !defined(_WIN32)
# define AS__EPERM (-EPERM)
#else
# define AS__EPERM (-4048)
#endif

#if defined(EPIPE) && !defined(_WIN32)
# define AS__EPIPE (-EPIPE)
#else
# define AS__EPIPE (-4047)
#endif

#if defined(EPROTO) && !defined(_WIN32)
# define AS__EPROTO (-EPROTO)
#else
# define AS__EPROTO (-4046)
#endif

#if defined(EPROTONOSUPPORT) && !defined(_WIN32)
# define AS__EPROTONOSUPPORT (-EPROTONOSUPPORT)
#else
# define AS__EPROTONOSUPPORT (-4045)
#endif

#if defined(EPROTOTYPE) && !defined(_WIN32)
# define AS__EPROTOTYPE (-EPROTOTYPE)
#else
# define AS__EPROTOTYPE (-4044)
#endif

#if defined(EROFS) && !defined(_WIN32)
# define AS__EROFS (-EROFS)
#else
# define AS__EROFS (-4043)
#endif

#if defined(ESHUTDOWN) && !defined(_WIN32)
# define AS__ESHUTDOWN (-ESHUTDOWN)
#else
# define AS__ESHUTDOWN (-4042)
#endif

#if defined(ESPIPE) && !defined(_WIN32)
# define AS__ESPIPE (-ESPIPE)
#else
# define AS__ESPIPE (-4041)
#endif

#if defined(ESRCH) && !defined(_WIN32)
# define AS__ESRCH (-ESRCH)
#else
# define AS__ESRCH (-4040)
#endif

#if defined(ETIMEDOUT) && !defined(_WIN32)
# define AS__ETIMEDOUT (-ETIMEDOUT)
#else
# define AS__ETIMEDOUT (-4039)
#endif

#if defined(ETXTBSY) && !defined(_WIN32)
# define AS__ETXTBSY (-ETXTBSY)
#else
# define AS__ETXTBSY (-4038)
#endif

#if defined(EXDEV) && !defined(_WIN32)
# define AS__EXDEV (-EXDEV)
#else
# define AS__EXDEV (-4037)
#endif

#if defined(EFBIG) && !defined(_WIN32)
# define AS__EFBIG (-EFBIG)
#else
# define AS__EFBIG (-4036)
#endif

#if defined(ENOPROTOOPT) && !defined(_WIN32)
# define AS__ENOPROTOOPT (-ENOPROTOOPT)
#else
# define AS__ENOPROTOOPT (-4035)
#endif

#if defined(ERANGE) && !defined(_WIN32)
# define AS__ERANGE (-ERANGE)
#else
# define AS__ERANGE (-4034)
#endif

#if defined(ENXIO) && !defined(_WIN32)
# define AS__ENXIO (-ENXIO)
#else
# define AS__ENXIO (-4033)
#endif

#if defined(EMLINK) && !defined(_WIN32)
# define AS__EMLINK (-EMLINK)
#else
# define AS__EMLINK (-4032)
#endif

/* EHOSTDOWN is not visible on BSD-like systems when _POSIX_C_SOURCE is
* defined. Fortunately, its value is always 64 so it's possible albeit
* icky to hard-code it.
*/
#if defined(EHOSTDOWN) && !defined(_WIN32)
# define AS__EHOSTDOWN (-EHOSTDOWN)
#elif defined(__APPLE__) || \
      defined(__DragonFly__) || \
      defined(__FreeBSD__) || \
      defined(__FreeBSD_kernel__) || \
      defined(__NetBSD__) || \
      defined(__OpenBSD__)
# define AS__EHOSTDOWN (-64)
#else
# define AS__EHOSTDOWN (-4031)
#endif

#if defined(EREMOTEIO) && !defined(_WIN32)
# define AS__EREMOTEIO (-EREMOTEIO)
#else
# define AS__EREMOTEIO (-4030)
#endif


#define AS_ERRNO_MAP(XX)                                                      \
  XX(E2BIG, "argument list too long")                                         \
  XX(EACCES, "permission denied")                                             \
  XX(EADDRINUSE, "address already in use")                                    \
  XX(EADDRNOTAVAIL, "address not available")                                  \
  XX(EAFNOSUPPORT, "address family not supported")                            \
  XX(EAGAIN, "resource temporarily unavailable")                              \
  XX(EAI_ADDRFAMILY, "address family not supported")                          \
  XX(EAI_AGAIN, "temporary failure")                                          \
  XX(EAI_BADFLAGS, "bad ai_flags value")                                      \
  XX(EAI_BADHINTS, "invalid value for hints")                                 \
  XX(EAI_CANCELED, "request canceled")                                        \
  XX(EAI_FAIL, "permanent failure")                                           \
  XX(EAI_FAMILY, "ai_family not supported")                                   \
  XX(EAI_MEMORY, "out of memory")                                             \
  XX(EAI_NODATA, "no address")                                                \
  XX(EAI_NONAME, "unknown node or service")                                   \
  XX(EAI_OVERFLOW, "argument buffer overflow")                                \
  XX(EAI_PROTOCOL, "resolved protocol is unknown")                            \
  XX(EAI_SERVICE, "service not available for socket type")                    \
  XX(EAI_SOCKTYPE, "socket type not supported")                               \
  XX(EALREADY, "connection already in progress")                              \
  XX(EBADF, "bad file descriptor")                                            \
  XX(EBUSY, "resource busy or locked")                                        \
  XX(ECANCELED, "operation canceled")                                         \
  XX(ECHARSET, "invalid Unicode character")                                   \
  XX(ECONNABORTED, "software caused connection abort")                        \
  XX(ECONNREFUSED, "connection refused")                                      \
  XX(ECONNRESET, "connection reset by peer")                                  \
  XX(EDESTADDRREQ, "destination address required")                            \
  XX(EEXIST, "file already exists")                                           \
  XX(EFAULT, "bad address in system call argument")                           \
  XX(EFBIG, "file too large")                                                 \
  XX(EHOSTUNREACH, "host is unreachable")                                     \
  XX(EINTR, "interrupted system call")                                        \
  XX(EINVAL, "invalid argument")                                              \
  XX(EIO, "i/o error")                                                        \
  XX(EISCONN, "socket is already connected")                                  \
  XX(EISDIR, "illegal operation on a directory")                              \
  XX(ELOOP, "too many symbolic links encountered")                            \
  XX(EMFILE, "too many open files")                                           \
  XX(EMSGSIZE, "message too long")                                            \
  XX(ENAMETOOLONG, "name too long")                                           \
  XX(ENETDOWN, "network is down")                                             \
  XX(ENETUNREACH, "network is unreachable")                                   \
  XX(ENFILE, "file table overflow")                                           \
  XX(ENOBUFS, "no buffer space available")                                    \
  XX(ENODEV, "no such device")                                                \
  XX(ENOENT, "no such file or directory")                                     \
  XX(ENOMEM, "not enough memory")                                             \
  XX(ENONET, "machine is not on the network")                                 \
  XX(ENOPROTOOPT, "protocol not available")                                   \
  XX(ENOSPC, "no space left on device")                                       \
  XX(ENOSYS, "function not implemented")                                      \
  XX(ENOTCONN, "socket is not connected")                                     \
  XX(ENOTDIR, "not a directory")                                              \
  XX(ENOTEMPTY, "directory not empty")                                        \
  XX(ENOTSOCK, "socket operation on non-socket")                              \
  XX(ENOTSUP, "operation not supported on socket")                            \
  XX(EPERM, "operation not permitted")                                        \
  XX(EPIPE, "broken pipe")                                                    \
  XX(EPROTO, "protocol error")                                                \
  XX(EPROTONOSUPPORT, "protocol not supported")                               \
  XX(EPROTOTYPE, "protocol wrong type for socket")                            \
  XX(ERANGE, "result too large")                                              \
  XX(EROFS, "read-only file system")                                          \
  XX(ESHUTDOWN, "cannot send after transport endpoint shutdown")              \
  XX(ESPIPE, "invalid seek")                                                  \
  XX(ESRCH, "no such process")                                                \
  XX(ETIMEDOUT, "connection timed out")                                       \
  XX(ETXTBSY, "text file is busy")                                            \
  XX(EXDEV, "cross-device link not permitted")                                \
  XX(UNKNOWN, "unknown error")                                                \
  XX(EOF, "end of file")                                                      \
  XX(ENXIO, "no such device or address")                                      \
  XX(EMLINK, "too many links")                                                \
  XX(EHOSTDOWN, "host is down")                                               \
  XX(EREMOTEIO, "remote I/O error")                                           \

typedef enum 
{
#define XX(code, _) AS_ ## code = AS__ ## code,
    AS_ERRNO_MAP(XX)
#undef XX
    AS_ERRNO_MAX = AS__EOF - 1
} uv_errno_t;

const char *as_err_name(int err);
const char *as_strerror(int err);
int as_translate_posix_error(int err);
//netErr参数在windows平台下才有效
int get_as_error(bool netErr = true);
//netErr参数在windows平台下才有效
const char *get_as_errmsg(bool netErr = true);

#endif /* AS_ERRNO_H_ */