#include "as_network_svr.h"

#if AS_APP_OS == AS_OS_LINUX || AS_APP_OS == AS_OS_ANDROID
#include <sys/epoll.h>
#endif

#if ((AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX)
#include <netinet/tcp.h>
#endif

#include <stdarg.h>

as_conn_mgr_log *g_pAsConnMgrLog = NULL;
#define MAX_CONN_LOG_LENTH 512
#define CONN_SECOND_IN_MS   1000
#define CONN_MS_IN_US   1000


#define _FL_ __FILE__, __LINE__

void CONN_WRITE_LOG(int32_t lLevel, const char *format, ...)
{
    if(NULL == g_pAsConnMgrLog)
    {
        return;
    }

    char buff[MAX_CONN_LOG_LENTH + 1];
    buff[0] = '\0';

    uint64_t ullTreadId = (uint64_t)as_thread_self();

    va_list args;
    va_start (args, format);
    int32_t lPrefix = snprintf (buff, MAX_CONN_LOG_LENTH, "errno:%d.thread(%llu):",
        CONN_ERRNO, ullTreadId);
    if(lPrefix < MAX_CONN_LOG_LENTH)
    {
        (void)vsnprintf (buff + lPrefix, (ULONG)(MAX_CONN_LOG_LENTH - lPrefix),
                    format, args);
    }
    buff[MAX_CONN_LOG_LENTH] = '\0';

    g_pAsConnMgrLog->writeLog(CONN_RUN_LOG, lLevel, buff, (int32_t)strlen(buff));
    va_end (args);
}


as_network_addr::as_network_addr()
{
    m_ulIpAddr = InvalidIp;
    m_usPort   = Invalidport;
    m_ailist   = NULL;
    strncpy(m_strAddr,"0.0.0.0",NETWORK_ADDR_STR_LEN);
}


as_network_addr::~as_network_addr()
{
    try
    {
        if (m_ailist != NULL)
          {
              freeaddrinfo(m_ailist);
              m_ailist = NULL;
          }
    }
    catch (...)
    {
    }
}

char* as_network_addr::get_host_addr()
{
    if (m_sa_family == AF_INET) 
    {
        unsigned char* p = (unsigned char*)&m_ulIpAddr;
        snprintf(m_strAddr, NETWORK_ADDR_STR_LEN, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
    }
	return &m_strAddr[0];
}
uint16_t as_network_addr::get_port_number()
{
    return m_usPort;
}

as_handle::as_handle()
{
    m_lSockFD = InvalidSocket;
    m_pHandleNode = NULL;
    m_ulEvents = EPOLLIN;
#if AS_APP_OS == AS_OS_WIN32 || AS_APP_OS == AS_OS_MAC || AS_APP_OS == AS_OS_IOS
    m_bReadSelected = AS_FALSE;
    m_bWriteSelected = AS_FALSE;
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    m_lEpfd = InvalidFd;
#endif //#if

    m_pMutexHandle = as_create_mutex();
}

as_handle::~as_handle()
{
    try
    {
        if(NULL != m_pHandleNode)
        {
            AS_DELETE(m_pHandleNode);
            m_pHandleNode = NULL;
        }

        if(NULL != m_pMutexHandle)
        {
            (void)as_destroy_mutex(m_pMutexHandle);
            m_pMutexHandle = NULL;
        }
    }
    catch (...)
    {
    }
}

int32_t as_handle::initHandle(void)
{
    if(!m_pMutexHandle)
        this->close();

    m_lSockFD = InvalidSocket;
    m_pHandleNode = NULL;
#if (AS_APP_OS  == AS_OS_LINUX || AS_APP_OS  == AS_OS_ANDROID)
    m_lEpfd = InvalidFd;
#endif
    m_ulEvents = EPOLLIN;
    if(!m_pMutexHandle && !(m_pMutexHandle = as_create_mutex()))
        return AS_ERROR_CODE_FAIL;
    
    return AS_ERROR_CODE_OK;
}

void as_handle::setHandleSend(AS_BOOLEAN bHandleSend)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }

    if(AS_FALSE == bHandleSend)
    {
        m_ulEvents = m_ulEvents & (~EPOLLOUT);
    }
    else
    {
        m_ulEvents = m_ulEvents | EPOLLOUT;
    }

    if((m_pHandleNode != NULL) && (m_lSockFD != InvalidSocket))
    {
#if (AS_APP_OS  == AS_OS_LINUX || AS_APP_OS  == AS_OS_ANDROID)
        struct epoll_event epEvent;
        memset(&epEvent, 0, sizeof(epEvent));
        epEvent.data.ptr = (void *)m_pHandleNode;
        epEvent.events = m_ulEvents;
        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_MOD, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_handle::setHandleSend: modify event fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }
#endif
    }

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }
}

void as_handle::setHandleRecv(AS_BOOLEAN bHandleRecv)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }

    if(AS_FALSE == bHandleRecv)
    {
        m_ulEvents = m_ulEvents & (~EPOLLIN);
    }
    else
    {
        m_ulEvents = m_ulEvents | EPOLLIN;
    }

    if((m_pHandleNode != NULL) && (m_lSockFD != InvalidSocket))
    {
#if (AS_APP_OS  == AS_OS_LINUX || AS_APP_OS  == AS_OS_ANDROID)
        struct epoll_event epEvent;
        memset(&epEvent, 0, sizeof(epEvent));
        epEvent.data.ptr = (void *)m_pHandleNode;
        epEvent.events = m_ulEvents;
        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_MOD, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_handle::setHandleRecv: modify event fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }
#endif
    }

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }
}

void as_handle::close(void)
{
    if (InvalidSocket != m_lSockFD)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }

    return;
}

as_network_handle::as_network_handle()
{
    m_lSockFD = InvalidSocket;
}


int32_t as_network_handle::initHandle(void)
{
    if (AS_ERROR_CODE_OK != as_handle::initHandle())
    {
        return AS_ERROR_CODE_FAIL;
    }
    m_lSockFD = InvalidSocket;
    return AS_ERROR_CODE_OK;
}

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
int32_t as_network_handle::sendMsg(const struct msghdr *pMsg)
{
    if (InvalidSocket == m_lSockFD)
    {
        return SendRecvError;
    }

    return ::sendmsg(m_lSockFD, pMsg, 0);
}
#endif

as_tcp_conn_handle::as_tcp_conn_handle()
{
    m_lConnStatus = enIdle;
}


as_tcp_conn_handle::~as_tcp_conn_handle()
{
    try
    {
        if (InvalidSocket != m_lSockFD)
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_handle::~as_handle: handle not released, "
                "m_lSockFD = %d, peer_ip(%s), peer_port(%d)", _FL_, m_lSockFD,
                m_peerAddr.get_host_addr(), ntohs(m_peerAddr.m_usPort));
            (void)CLOSESOCK((SOCKET)m_lSockFD);
            m_lSockFD = InvalidSocket;
        }
    }
    catch (...)
    {
    }
}


int32_t as_tcp_conn_handle::initHandle(void)
{
    if (AS_ERROR_CODE_OK != as_network_handle::initHandle())
    {
        return AS_ERROR_CODE_FAIL;
    }

    m_lConnStatus = enIdle;
    return AS_ERROR_CODE_OK;
}

int32_t as_tcp_conn_handle::conn(const as_network_addr *pLocalAddr,
    const as_network_addr *pPeerAddr, const EnumSyncAsync bSyncConn, ULONG ulTimeOut)
{
    m_lConnStatus = enConnFailed;

    int32_t lSockFd = (int32_t)socket(pPeerAddr->m_ailist->ai_family, SOCK_STREAM, 0);
    if(lSockFd < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "opening client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
    //setSendBufSize
    int32_t lSendBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lSendBufLength = sizeof(lSendBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize,
        lSendBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
    //setRecvBufSize
    int32_t lRecvBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lRecvBufLength = sizeof(lRecvBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize,
        lRecvBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    int32_t flag = 1;
    if(setsockopt((SOCKET)lSockFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "set TCP_NODELAY client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    //setReuseAddr();
    int32_t lReuseAddrFlag = 1;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
        sizeof(lReuseAddrFlag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    if(((ULONG)(pLocalAddr->m_ulIpAddr) != InvalidIp) && ( pLocalAddr->m_usPort != Invalidport))
    {
        struct sockaddr_in  serverAddr;
        memset((char *)&serverAddr, 0, (int32_t)sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = (uint32_t)pLocalAddr->m_ulIpAddr;
        serverAddr.sin_port = pLocalAddr->m_usPort;
        errno = 0;
        if (0 > bind ((SOCKET)lSockFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)))
        {
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
            char szServerAddr[INET_ADDRSTRLEN];
            if (NULL != inet_ntop(AF_INET, &serverAddr.sin_addr, szServerAddr,
                sizeof(szServerAddr)))
#elif AS_APP_OS == AS_OS_WIN32
            char *szServerAddr = inet_ntoa(serverAddr.sin_addr);
            if (NULL != szServerAddr)
#endif
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "Can not Bind Data_Sock %s:%d", _FL_,
                    szServerAddr, ntohs(serverAddr.sin_port));
            }
            else
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "Can not Bind Data_Sock %d:%d", _FL_,
                    serverAddr.sin_addr.s_addr, ntohs(serverAddr.sin_port));
            }

            (void)CLOSESOCK((SOCKET)lSockFd);
            return AS_ERROR_CODE_FAIL;
        }
    }

    errno = 0;
    if((enAsyncOp == bSyncConn) || (ulTimeOut > 0))
    {
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
        if(fcntl(lSockFd, F_SETFL, fcntl(lSockFd, F_GETFL)|O_NONBLOCK) < 0)
#elif AS_APP_OS == AS_OS_WIN32
        ULONG ulNoBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)lSockFd,(int32_t)(int32_t)FIONBIO,&ulNoBlock))
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "fcntl client socket error(%d)", _FL_, CONN_ERRNO);
            (void)CLOSESOCK((SOCKET)lSockFd);
            return AS_ERROR_CODE_FAIL;
        }
        setHandleSend(AS_TRUE);
    }

    int32_t lRetVal = ::connect((SOCKET)lSockFd, pPeerAddr->m_ailist->ai_addr, pPeerAddr->m_ailist->ai_addrlen);
    if( lRetVal < 0)
    {
        if((enSyncOp == bSyncConn) && (0 == ulTimeOut))
        {
            (void)CLOSESOCK((SOCKET)lSockFd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "SyncConn server fail. error(%d):%s",
                _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
            return AS_ERROR_CODE_FAIL;

        }

        auto err = CONN_ERRNO;
        if ((EINPROGRESS != CONN_ERRNO) && (EWOULDBLOCK != CONN_ERRNO))
        {
            (void)CLOSESOCK((SOCKET)lSockFd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "AsyncConn server fail. error(%d):%s", _FL_,
                CONN_ERRNO, strerror(CONN_ERRNO));
            return AS_ERROR_CODE_FAIL;
        }

        if(enSyncOp == bSyncConn)
        {
            fd_set    fdWriteReady;
            struct timeval waitTime;
            waitTime.tv_sec = (long)ulTimeOut/CONN_SECOND_IN_MS;
            waitTime.tv_usec = (ulTimeOut%CONN_SECOND_IN_MS)*CONN_MS_IN_US;
            FD_ZERO(&fdWriteReady);
            FD_SET((SOCKET)lSockFd, &fdWriteReady);
            int32_t lSelectResult = select(FD_SETSIZE, (fd_set*)0, &fdWriteReady, (fd_set*)0, &waitTime);
            if(lSelectResult <= 0)
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "wait client socket(%d) time out", _FL_, lSockFd);
                (void)CLOSESOCK((SOCKET)lSockFd);
                return AS_ERROR_CODE_FAIL;
            }
            int32_t lErrorNo = 0;
            socklen_t len = sizeof(lErrorNo);
            if (getsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_ERROR,
                (SOCK_OPT_TYPE *)&lErrorNo, &len) < 0)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "getsockopt of sockfd(%d) has wrong when wait client",
                    _FL_, lSockFd);
                (void)CLOSESOCK((SOCKET)lSockFd);
                return AS_ERROR_CODE_FAIL;
            }
            else if (lErrorNo != 0)
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "wait client: socket(%d) connect fail", _FL_, lSockFd);
                (void)CLOSESOCK((SOCKET)lSockFd);
                return AS_ERROR_CODE_FAIL;
            }

            CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
                "connect server OK. socket id = %d", _FL_, lSockFd);
            m_lConnStatus = enConnected;
        }
    }
    else
    {
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "connect server OK. socket id = %d", _FL_, lSockFd);
    }

    if((enAsyncOp == bSyncConn) || (ulTimeOut > 0))
    {
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
        if(fcntl(lSockFd, F_SETFL, fcntl(lSockFd, F_GETFL&(~O_NONBLOCK))) < 0)
#elif AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = 0;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)lSockFd,(int32_t)FIONBIO,&ulBlock))
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "fcntl client socket error(%d)", _FL_, CONN_ERRNO);
            (void)CLOSESOCK((SOCKET)lSockFd);
            return AS_ERROR_CODE_FAIL;
        }

        if(enAsyncOp == bSyncConn)
        {
            m_lConnStatus = enConnecting;
        }
    }
    else
    {
        m_lConnStatus = enConnected;
    }

    m_lSockFD = lSockFd;

	m_peerAddr.m_ulIpAddr = pPeerAddr->m_ulIpAddr;
    m_peerAddr.m_usPort= pPeerAddr->m_usPort;
    m_peerAddr.m_sa_family = pPeerAddr->m_sa_family;
    memcpy(m_peerAddr.m_strAddr, pPeerAddr->m_strAddr, NETWORK_ADDR_STR_LEN);

    CONN_WRITE_LOG(CONN_INFO, (char *)"FILE(%s)LINE(%d): "
        "as_tcp_conn_handle::conn: connect success, "
        "m_lSockFD = %d, peer_ip(%s) ,peer_port(%d)", _FL_, m_lSockFD
        , m_peerAddr.get_host_addr(), ntohs(m_peerAddr.m_usPort));

    return AS_ERROR_CODE_OK;
}

int32_t as_tcp_conn_handle::send(const char *pArrayData, const ULONG ulDataSize,
    const EnumSyncAsync bSyncSend)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_handle::send: socket is invalid, send fail", _FL_);
        return SendRecvError;
    }

    errno = 0;
    int32_t lBytesSent = 0;
    if(enSyncOp == bSyncSend)
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::send((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, MSG_NOSIGNAL);
    }
    else
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::send((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            MSG_DONTWAIT|MSG_NOSIGNAL);
        setHandleSend(AS_TRUE);
    }

    if (lBytesSent < 0)
    {
        if ((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO) )
        {
            return 0;
        }

        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_handle::send to peer(IP:%s, Port:%d) "
            "Error(%d): %s",  _FL_, m_peerAddr.get_host_addr(),
           ntohs(m_peerAddr.m_usPort), CONN_ERRNO, strerror(CONN_ERRNO));

        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
        return SendRecvError;
    }

    return lBytesSent;
}

int32_t as_tcp_conn_handle::recv(char *pArrayData, as_network_addr *pPeerAddr,
    const ULONG ulDataSize, const EnumSyncAsync bSyncRecv)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_handle::recv: socket is invalid, recv fail", _FL_);
        return SendRecvError;
    }

    errno = 0;
    int32_t lBytesRecv = 0;
    if(enSyncOp == bSyncRecv)
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = ::recv((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, MSG_WAITALL);
    }
    else
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = ::recv((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, MSG_DONTWAIT);
    }

    if (0 == lBytesRecv)
    {
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): recv EOF!", _FL_);
        return SendRecvClose;
    }

    if (lBytesRecv < 0)
    {
#if AS_APP_OS == AS_OS_WIN32
        if (EWOULDBLOCK == CONN_ERRNO)
#else 
        if ((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO) || (EINTR == CONN_ERRNO))
#endif
        {
            return 0;
        }
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "recv error. Error(%d): %s", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        return SendRecvError;
    }

    pPeerAddr->m_ulIpAddr = m_peerAddr.m_ulIpAddr;
    pPeerAddr->m_usPort = m_peerAddr.m_usPort;

    return lBytesRecv;
}

int32_t as_tcp_conn_handle::recvWithTimeout(char *pArrayData, as_network_addr *pPeerAddr,
    const ULONG ulDataSize, const ULONG ulTimeOut, const ULONG ulSleepTime)
{
    (void)ulSleepTime;
    int32_t lRecvBytes = 0;
    ULONG ulTotalRecvBytes = 0;
    ULONG ulWaitTime = ulTimeOut;
    errno = 0;
#if AS_APP_OS == AS_OS_WIN32

    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &ulWaitTime, sizeof(int)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX

    struct timeval recvWaitTime;
    recvWaitTime.tv_sec = ulWaitTime/CONN_SECOND_IN_MS;
    recvWaitTime.tv_usec = (ulWaitTime%CONN_SECOND_IN_MS)*CONN_MS_IN_US;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#endif

    if(NULL == pArrayData)
    {
        return SendRecvError;
    }

    while(ulTotalRecvBytes<ulDataSize)
    {
        lRecvBytes = this->recv(pArrayData+ulTotalRecvBytes,pPeerAddr, ulDataSize-ulTotalRecvBytes, enSyncOp);
        if(lRecvBytes < 0)
        {
            break;
        }

        ulTotalRecvBytes += (uint32_t)lRecvBytes;
    }


    if(lRecvBytes < 0)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_handle::recvWithTimeout: socket closed when receive. "
            "m_lSockFD = %d, peer_ip(%s), peer_port(%d) "
            "errno = %d, error: %s", _FL_, m_lSockFD,
            m_peerAddr.get_host_addr(), ntohs(m_peerAddr.m_usPort),
            CONN_ERRNO, strerror(CONN_ERRNO) );
        if(CONN_ERR_TIMEO == CONN_ERRNO)
        {
            return SendRecvErrorTIMEO;
        }
        if(CONN_ERR_EBADF == CONN_ERRNO)
        {
            return SendRecvErrorEBADF;
        }
        return SendRecvError;
    }

    if(ulTotalRecvBytes <  ulDataSize)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_handle::recvWithTimeout: recv time out. "
            "m_lSockFD = %d, peer_ip(%s), peer_port(%d) recv_msg_len(%lu)"
            "ulDataSize(%lu) errno = %d, error: %s", _FL_, m_lSockFD,
            m_peerAddr.get_host_addr(), ntohs(m_peerAddr.m_usPort),
            ulTotalRecvBytes, ulDataSize,CONN_ERRNO, strerror(CONN_ERRNO) );
        return SendRecvError;
    }

#if AS_APP_OS == AS_OS_WIN32

    ulWaitTime = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &ulWaitTime, sizeof(int)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX

    recvWaitTime.tv_sec = 0;
    recvWaitTime.tv_usec = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
        (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }
#endif

    return (int32_t)ulTotalRecvBytes;

}

ConnStatus as_tcp_conn_handle::getStatus(void) const
{
    return m_lConnStatus;
}

void as_tcp_conn_handle::close(void)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }

    if (InvalidSocket != m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_handle::close: close connection, "
            "m_lSockFD = %d, peer_ip(%s), "
            "peer_port(%d) this(0x%x) m_pHandleNode(0x%x)",
            _FL_, m_lSockFD,
            m_peerAddr.get_host_addr(), ntohs(m_peerAddr.m_usPort), this,
            this->m_pHandleNode);

        //The close of an fd will cause it to be removed from
        //all epoll sets automatically.
#if (AS_APP_OS  == AS_OS_LINUX || AS_APP_OS  == AS_OS_ANDROID)
        struct epoll_event epEvent;
        memset(&epEvent, 0, sizeof(epEvent));
        epEvent.data.ptr = (void *)NULL;
        epEvent.events = (EPOLLIN | EPOLLOUT);
        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_MOD, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_handle::setHandleSend: modify event fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }

        if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_DEL, m_lSockFD, &epEvent))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_tcp_conn_handle::close: epoll_ctl EPOLL_CTL_DEL fail, "
                "m_lSockFD = %d", _FL_, m_lSockFD);
        }
#endif
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }
    m_lConnStatus = enClosed;

    as_handle::close();

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }

    return;
}

as_tcp_monitor_handle::as_tcp_monitor_handle()
{

}
as_tcp_monitor_handle::~as_tcp_monitor_handle()
{

}

void as_tcp_monitor_handle::close(void)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }

    if (InvalidSocket != m_lSockFD)
    {
        m_lSockFD = InvalidSocket;
    }
    m_lConnStatus = enClosed;

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }
    return;
}

int32_t as_udp_sock_handle::createSock(const as_network_addr *pLocalAddr,
                                         const as_network_addr *pMultiAddr)
{
    int32_t lSockFd = (int32_t)InvalidSocket;
    if ((lSockFd = (int32_t)socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "create udp socket failed, errno = %d, Msg = %s",
            _FL_, CONN_ERRNO, strerror(CONN_ERRNO));

        return AS_ERROR_CODE_FAIL;
    }

    struct sockaddr_in localAddr;
    memset((char *)&localAddr, 0, (int32_t)sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    if(NULL != pMultiAddr)
    {
        localAddr.sin_addr.s_addr =  INADDR_ANY;
    }
    else
    {
        localAddr.sin_addr.s_addr = (UINT)pLocalAddr->m_ulIpAddr;
    }
    localAddr.sin_port = pLocalAddr->m_usPort;

    if (0 > bind((SOCKET)lSockFd, (struct sockaddr *)&localAddr, sizeof(localAddr)))
    {
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
        char szLocalAddr[INET_ADDRSTRLEN];
        if (NULL != inet_ntop(AF_INET, &localAddr.sin_addr, szLocalAddr,
            sizeof(szLocalAddr)))
#elif AS_APP_OS == AS_OS_WIN32
        char *szLocalAddr = inet_ntoa(localAddr.sin_addr);
        if (NULL == szLocalAddr)
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %s:%d", _FL_, szLocalAddr,
                ntohs(localAddr.sin_port));
        }
        else
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %d:%d", _FL_,
                localAddr.sin_addr.s_addr, ntohs(localAddr.sin_port));
        }
        (void)CLOSESOCK((SOCKET)lSockFd);
        return AS_ERROR_CODE_FAIL;
    }

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    int32_t lReuseAddrFlag = 1;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
        sizeof(lReuseAddrFlag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
#endif

    int32_t lSendBufSize = DEFAULT_UDP_SENDRECV_SIZE;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize, sizeof(lSendBufSize)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
    //setRecBufSize
    int32_t lRecvBufSize = DEFAULT_UDP_SENDRECV_SIZE;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize, sizeof(lRecvBufSize)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    if(NULL != pMultiAddr)
    {
        struct ip_mreq mreq;
        mreq.imr_multiaddr.s_addr = pMultiAddr->m_ulIpAddr;
        mreq.imr_interface.s_addr = pLocalAddr->m_ulIpAddr;
        if(setsockopt((SOCKET)lSockFd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                             (char *)&mreq, sizeof(mreq))< 0)
        {
            (void)CLOSESOCK((SOCKET)lSockFd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "set IPPROTO_IP IP_ADD_MEMBERSHIP error(%d)", _FL_, CONN_ERRNO);
            return AS_ERROR_CODE_FAIL;
        }
    }
#endif
    m_lSockFD = lSockFd;
    return AS_ERROR_CODE_OK;
}

int32_t as_udp_sock_handle::send(const as_network_addr *pPeerAddr, const char *pArrayData,
         const ULONG ulDataSize, const EnumSyncAsync bSyncSend)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_udp_sock_handle::send: socket is invalid, send fail", _FL_);
        return SendRecvError;
    }

    struct sockaddr_in peerAddr;
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_addr.s_addr = (UINT)pPeerAddr->m_ulIpAddr;
    peerAddr.sin_port = pPeerAddr->m_usPort;

    errno = 0;
    int32_t lBytesSent = 0;
    if(enSyncOp == bSyncSend)
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::sendto((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            (int32_t)MSG_NOSIGNAL, (const struct sockaddr *)&peerAddr, sizeof(peerAddr));
    }
    else
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesSent = ::sendto((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            (int32_t) MSG_DONTWAIT|MSG_NOSIGNAL,
            (const struct sockaddr *)&peerAddr, sizeof(peerAddr));
        setHandleSend(AS_TRUE);
    }

    if (lBytesSent < 0)
    {
        if((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO))
        {
            return 0;
        }
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_udp_sock_handle::send (%d)bytes to peer(IP:0x%x, Port:%d) Error(%d): %s",
            _FL_, ulDataSize, ntohl((ULONG)(pPeerAddr->m_ulIpAddr)),
            ntohs(pPeerAddr->m_usPort),
            CONN_ERRNO, strerror(CONN_ERRNO));

        return SendRecvError;
    }

    return lBytesSent;
}

int32_t as_udp_sock_handle::recv(char *pArrayData, as_network_addr *pPeerAddr,
    const ULONG ulDataSize, const EnumSyncAsync bSyncRecv)
{
    if (InvalidSocket == m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_udp_sock_handle::recv: socket is invalid, recv fail", _FL_);
        return SendRecvError;
    }

    errno = 0;
    struct sockaddr_in  peerAddr;
    socklen_t iFromlen = sizeof(peerAddr);
    int32_t lBytesRecv = 0;
    if(enSyncOp == bSyncRecv)
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_FALSE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket Block fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = recvfrom((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize,
            MSG_WAITALL, (struct sockaddr *)&peerAddr, &iFromlen);
    }
    else
    {
#if AS_APP_OS == AS_OS_WIN32
        ULONG ulBlock = AS_TRUE;
        if (SOCKET_ERROR == ioctlsocket((SOCKET)m_lSockFD,(int32_t)FIONBIO,&ulBlock))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "Set Socket NoBlock fail.", _FL_);
            return SendRecvError;
        }
#endif
        lBytesRecv = recvfrom((SOCKET)m_lSockFD, pArrayData, (int)ulDataSize, 0,
            (struct sockaddr *)&peerAddr, &iFromlen);
    }

    if (0 == lBytesRecv)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): recv EOF!", _FL_);
        return SendRecvError;
    }

    if (lBytesRecv < 0)
    {
        if((EWOULDBLOCK == CONN_ERRNO) || (EAGAIN == CONN_ERRNO))
        {
            return 0;
        }
        CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "recv error. Error(%d): %s", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        return SendRecvError;
    }

    pPeerAddr->m_ulIpAddr = (LONG)peerAddr.sin_addr.s_addr;
    pPeerAddr->m_usPort = peerAddr.sin_port;

    //setHandleRecv(AS_TRUE);

    return lBytesRecv;
}

int32_t as_udp_sock_handle::recvWithTimeout(char *pArrayData, as_network_addr *pPeerAddr,
    const ULONG ulDataSize, const ULONG ulTimeOut, const ULONG ulSleepTime)
{
    (void)ulSleepTime;
    int32_t lRecvBytes = 0;
    ULONG ulTotalRecvBytes = 0;
    ULONG ulWaitTime = ulTimeOut;
    errno = 0;
#if AS_APP_OS == AS_OS_WIN32

    ULONG recvWaitTime;
    recvWaitTime = ulWaitTime;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX

    struct timeval recvWaitTime;
    recvWaitTime.tv_sec = ulWaitTime/CONN_SECOND_IN_MS;
    recvWaitTime.tv_usec = (ulWaitTime%CONN_SECOND_IN_MS)*CONN_MS_IN_US;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#endif

    lRecvBytes = this->recv(pArrayData, pPeerAddr, ulDataSize, enSyncOp);
    if(lRecvBytes < 0)
    {
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "as_udp_sock_handle::recvWithTimeout: socket closed when receive. "
            "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d) "
            "errno = %d, error: %s", _FL_, m_lSockFD,
            ntohl((ULONG)(pPeerAddr->m_ulIpAddr)), ntohs(pPeerAddr->m_usPort),
            CONN_ERRNO, strerror(CONN_ERRNO) );
        return SendRecvError;
    }

    ulTotalRecvBytes += (ULONG)lRecvBytes;

#if AS_APP_OS == AS_OS_WIN32

    recvWaitTime = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }

#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX

    recvWaitTime.tv_sec = 0;
    recvWaitTime.tv_usec = 0;
    if(setsockopt((SOCKET)m_lSockFD, SOL_SOCKET, SO_RCVTIMEO,
                  (char *) &recvWaitTime, sizeof(recvWaitTime)) < 0)
    {
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setsockopt socket SO_RCVTIMEO  error(%d)\n", _FL_, CONN_ERRNO);
        return SendRecvError;
    }
#endif

    return (int32_t)ulTotalRecvBytes;
}

void as_udp_sock_handle::close(void)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }
    if (InvalidSocket != m_lSockFD)
    {
        //The close of an fd will cause it to be removed from
        //all epoll sets automatically.
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }

    as_handle::close();

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }

    return;
}

int32_t as_tcp_server_handle::listen(const as_network_addr *pLocalAddr)
{
    int32_t lSockFd = (int32_t)socket(AF_INET, SOCK_STREAM, 0);
    if(lSockFd < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "opening client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    //setSendBufSize
    int32_t lSendBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lSendBufLength = sizeof(lSendBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize, lSendBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

    //setRecBufSize
    int32_t lRecvBufSize = DEFAULT_TCP_SENDRECV_SIZE;
    socklen_t lRecvBufLength = sizeof(lRecvBufSize);
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize,
        lRecvBufLength) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }

#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    //setReuseAddr();
    int32_t lReuseAddrFlag = 1;
    if(setsockopt((SOCKET)lSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
        sizeof(lReuseAddrFlag)) < 0)
    {
        (void)CLOSESOCK((SOCKET)lSockFd);
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "setsockopt client socket error(%d)", _FL_, CONN_ERRNO);
        return AS_ERROR_CODE_FAIL;
    }
#endif

    struct sockaddr_in  serverAddr;
    memset((char *)&serverAddr, 0, (int32_t)sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = (UINT)pLocalAddr->m_ulIpAddr;
    serverAddr.sin_port = pLocalAddr->m_usPort;
    errno = 0;
    if (0 > bind ((SOCKET)lSockFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)))
    {
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
        char szServerAddr[INET_ADDRSTRLEN];
        if (NULL != inet_ntop(AF_INET, &serverAddr.sin_addr, szServerAddr,
            sizeof(szServerAddr)))
#elif AS_APP_OS == AS_OS_WIN32
        char *szServerAddr = inet_ntoa(serverAddr.sin_addr);
        if (NULL != szServerAddr)
#endif
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %s:%d", _FL_,
                szServerAddr, ntohs(serverAddr.sin_port));
        }
        else
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "Can not Bind Data_Sock %d:%d", _FL_,
                serverAddr.sin_addr.s_addr, ntohs(serverAddr.sin_port));
        }
        (void)CLOSESOCK((SOCKET)lSockFd);
        return AS_ERROR_CODE_FAIL;
    }

    errno = 0;
    if(::listen((SOCKET)lSockFd, MAX_LISTEN_QUEUE_SIZE) < 0)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "listen Error(%d):%s.", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        (void)CLOSESOCK((SOCKET)lSockFd);
        return AS_ERROR_CODE_FAIL;
    }

    m_lSockFD = lSockFd;
    return AS_ERROR_CODE_OK;
}

void as_tcp_server_handle::close(void)
{
    if(m_pMutexHandle != NULL)
    {
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexHandle))
        {
            return;
        }
    }
    if (InvalidSocket != m_lSockFD)
    {
        //The close of an fd will cause it to be removed from
        //all epoll sets automatically.
        (void)CLOSESOCK((SOCKET)m_lSockFD);
        m_lSockFD = InvalidSocket;
    }

    as_handle::close();

    if(m_pMutexHandle != NULL)
    {
        (void)as_mutex_unlock(m_pMutexHandle);
    }

    return;
}


as_handle_manager::as_handle_manager()
{
    m_pMutexListOfHandle = NULL;
#if AS_APP_OS == AS_OS_WIN32 || AS_APP_OS == AS_OS_MAC || AS_APP_OS == AS_OS_IOS
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
    m_stSelectPeriod.tv_sec = 0;
    m_stSelectPeriod.tv_usec = 0;
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    m_lEpfd = InvalidFd;
    memset(m_epEvents, 0, sizeof(m_epEvents));    
#endif
    m_ulSelectPeriod = DEFAULT_SELECT_PERIOD;
    m_pSVSThread = NULL;
    m_bExit = AS_FALSE;
    memset(m_szMgrType, 0, sizeof(m_szMgrType));
}

as_handle_manager::~as_handle_manager()
{
    try
    {
    #if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::~as_handle_manager: "
            "manager type: %s. thread = %d.",
            _FL_, m_szMgrType, as_thread_self());
    #elif AS_APP_OS == AS_OS_WIN32
        CONN_WRITE_LOG(CONN_WARNING,   (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::~as_handle_manager: "
            "manager type: %s. thread = %d ", _FL_, m_szMgrType, as_thread_self());
    #endif

        ListOfHandleIte itListOfHandle = m_listHandle.begin();
        while(itListOfHandle != m_listHandle.end())
        {
            if((*itListOfHandle)->m_pHandle != NULL)
            {
                (*itListOfHandle)->m_pHandle->close();
            }
    #if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX

    #endif
            AS_DELETE(*itListOfHandle);
            ++itListOfHandle;
        }

    #if (AS_APP_OS  == AS_OS_LINUX || AS_APP_OS  == AS_OS_ANDROID)
        if (m_lEpfd != InvalidFd)
        {
            (void)CLOSESOCK(m_lEpfd);
            m_lEpfd = InvalidFd;
        }
    #endif

        if(m_pSVSThread != NULL)
        {
            free(m_pSVSThread);
        }
        m_pSVSThread = NULL;

        if(m_pMutexListOfHandle != NULL)
        {
            if(AS_ERROR_CODE_OK == as_destroy_mutex(m_pMutexListOfHandle))
            {
                m_pMutexListOfHandle = NULL;
            }
        }

        m_pMutexListOfHandle = NULL;
    }
    catch (...)
    {
    }
}

int32_t as_handle_manager::init(const ULONG ulSelectPeriod)
{
    if (0 == ulSelectPeriod)
    {
        m_ulSelectPeriod = DEFAULT_SELECT_PERIOD;
    }
    else
    {
        m_ulSelectPeriod = ulSelectPeriod;
    }
#if AS_APP_OS == AS_OS_WIN32 || AS_APP_OS == AS_OS_MAC  || AS_APP_OS == AS_OS_IOS
    m_stSelectPeriod.tv_sec = (int32_t)(ulSelectPeriod / CONN_SECOND_IN_MS);
    m_stSelectPeriod.tv_usec = (ulSelectPeriod % CONN_SECOND_IN_MS) * CONN_MS_IN_US;
    FD_ZERO(&m_readSet);
    FD_ZERO(&m_writeSet);
#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    m_lEpfd = epoll_create(MAX_EPOLL_FD_SIZE);

    if(m_lEpfd < 0)
    {
        m_lEpfd = InvalidFd;
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::init: create file handle for epoll fail. "
            "manager type: %s", _FL_, m_szMgrType);
        return AS_ERROR_CODE_FAIL;
    }    
#endif

    m_pMutexListOfHandle = as_create_mutex();
    if(NULL == m_pMutexListOfHandle)
    {

#if (AS_APP_OS  == AS_OS_LINUX || AS_APP_OS  == AS_OS_ANDROID)
        close(m_lEpfd);
        m_lEpfd = InvalidFd;
#endif
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::init: create m_pMutexListOfHandle fail. "
            "manager type: %s", _FL_, m_szMgrType);
        return AS_ERROR_CODE_FAIL;
    }

    return AS_ERROR_CODE_OK;
}


int32_t as_handle_manager::run()
{
    errno = 0;
    if (AS_ERROR_CODE_OK != as_create_thread((AS_THREAD_FUNC)invoke, (void *)this,
        &m_pSVSThread, AS_DEFAULT_STACK_SIZE))
    {
        CONN_WRITE_LOG(CONN_ERROR,  (char *)"FILE(%s)LINE(%d): "
            "Create play thread failed. manager type: %s. error(%d):%s",
            _FL_, m_szMgrType, CONN_ERRNO, strerror(CONN_ERRNO));
        return AS_ERROR_CODE_FAIL;
    }
    CONN_WRITE_LOG(CONN_INFO,  (char *)"FILE(%s)LINE(%d): "
        "as_create_thread: manager type: %s. create thread %d", _FL_,
        m_szMgrType, m_pSVSThread->pthead);

    return AS_ERROR_CODE_OK;
}

void *as_handle_manager::invoke(void *argc)
{
    as_handle_manager *pHandleManager = (as_handle_manager *)argc;
    CONN_WRITE_LOG(CONN_INFO, (char *)"FILE(%s)LINE(%d): %s invoke mainLoop",
         _FL_, pHandleManager->m_szMgrType);
    pHandleManager->mainLoop();
    as_thread_exit(NULL);
    return NULL;
}

#if AS_APP_OS == AS_OS_WIN32 || AS_APP_OS == AS_OS_MAC  || AS_APP_OS == AS_OS_IOS
void as_handle_manager::mainLoop()
{
    while(AS_FALSE == m_bExit)
    {
        errno = 0;
        int32_t lWaitFds = 0;
        int32_t lMaxfd   = -1;
        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
           break;
        }

        FD_ZERO(&m_readSet);
        FD_ZERO(&m_writeSet);
        lMaxfd   = -1;
        m_stSelectPeriod.tv_sec =  (int32_t)(m_ulSelectPeriod / CONN_SECOND_IN_MS);
        m_stSelectPeriod.tv_usec = (m_ulSelectPeriod % CONN_SECOND_IN_MS) * CONN_MS_IN_US;

        ListOfHandleIte itListOfHandle = m_listHandle.begin();
        while(itListOfHandle != m_listHandle.end())
        {
            as_handle_node *pHandleNode = NULL;
            as_handle *pHandle = NULL;
            int32_t lSockFd = InvalidSocket;

            if(AS_TRUE == (*itListOfHandle)->m_bRemoved)
            {
                pHandleNode = *itListOfHandle;
                itListOfHandle = m_listHandle.erase(itListOfHandle);
                AS_DELETE(pHandleNode);
                continue;
            }
            else
            {
                pHandleNode = *itListOfHandle;
                pHandle = pHandleNode->m_pHandle;
                lSockFd = pHandle->m_lSockFD;
                pHandle->m_bReadSelected = AS_FALSE;
                pHandle->m_bWriteSelected = AS_FALSE;
                if(lSockFd != InvalidSocket)
                {
                    ULONG ulEvent = pHandle->getEvents();
                    if (EPOLLIN == (ulEvent & EPOLLIN))
                    {
                        if(!FD_ISSET(lSockFd,&m_readSet))
                        {
                            FD_SET((SOCKET)lSockFd,&m_readSet);
                            if(lMaxfd < lSockFd) {
                                lMaxfd = lSockFd;
                            }
                            pHandle->m_bReadSelected = AS_TRUE;
                        }
                    }

                    if (EPOLLOUT == (ulEvent & EPOLLOUT))
                    {
                        if(!FD_ISSET(lSockFd,&m_writeSet))
                        {
                            FD_SET((SOCKET)lSockFd,&m_writeSet);
                            if(lMaxfd < lSockFd) {
                                 lMaxfd = lSockFd;
                             }
                            pHandle->m_bWriteSelected = AS_TRUE;
                        }
                    }
                }

            }
            ++itListOfHandle;
        }
        (void)as_mutex_unlock(m_pMutexListOfHandle);

#if AS_APP_OS == AS_OS_WIN32
        if ((0 == m_readSet.fd_count) && (0 == m_writeSet.fd_count))
        {
            as_sleep(1);
            continue;
        }
        else
        {
            if (0 == m_readSet.fd_count)
            {
                lWaitFds = select(lMaxfd + 1, NULL, &m_writeSet, NULL, &m_stSelectPeriod);
            }
            else
            {
                if (0 == m_writeSet.fd_count)
                {
                    lWaitFds = select(lMaxfd + 1, &m_readSet, NULL, NULL, &m_stSelectPeriod);
                }
                else
                {
#endif
                    lWaitFds = select(lMaxfd + 1, &m_readSet,&m_writeSet,NULL,&m_stSelectPeriod);
#if AS_APP_OS == AS_OS_WIN32
                }
            }
        }
#endif
        if (0 == lWaitFds)
        {
            continue;
        }
#if AS_APP_OS == AS_OS_WIN32
        if (SOCKET_ERROR == lWaitFds)
#else
        if (0 > lWaitFds)
#endif
        {
            CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
                "select failed: manager type: %s. errno = %d",
                _FL_, m_szMgrType, CONN_ERRNO);
            //break;
            continue;
        }

        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
           break;
        }

        as_handle_node *pHandleNode = NULL;
        for(ListOfHandleIte it = m_listHandle.begin(); it != m_listHandle.end(); ++it)
        {
            pHandleNode = *it;
            if (AS_TRUE == pHandleNode->m_bRemoved)
            {
                continue;
            }

            as_handle *pHandle = pHandleNode->m_pHandle;
            if (pHandle->m_lSockFD != InvalidSocket)
            {
                if (FD_ISSET(pHandle->m_lSockFD,&m_readSet) && (AS_TRUE == pHandle->m_bReadSelected))
                {
                    this->checkSelectResult(enEpollRead, pHandleNode->m_pHandle);
                }

                if (FD_ISSET(pHandle->m_lSockFD,&m_writeSet) && (AS_TRUE == pHandle->m_bWriteSelected))
                {
                    this->checkSelectResult(enEpollWrite, pHandleNode->m_pHandle);
                }
            }
        }
        (void)as_mutex_unlock(m_pMutexListOfHandle);
    }

    return;
}

#elif (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX

void as_handle_manager::mainLoop()
{
    while(AS_FALSE == m_bExit)
    {
        errno = 0;
        int32_t lWaitFds = epoll_wait(m_lEpfd, m_epEvents, EPOLL_MAX_EVENT,
            (int32_t)m_ulSelectPeriod);
        if (0 == lWaitFds )
        {
            continue;
        }

        if (0 > lWaitFds )
        {
            if(EINTR == CONN_ERRNO)
            {
                continue;
            }

            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "epoll_wait failed: manager type: %s. errno(%d):%s",
                _FL_, m_szMgrType, CONN_ERRNO, strerror(CONN_ERRNO));
            break;
        }

        if(AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
           break;
        }

        as_handle_node *pHandleNode = NULL;
        for(int32_t i = 0; i < lWaitFds; ++i)
        {
            pHandleNode = (as_handle_node *)(m_epEvents[i].data.ptr);
            if(NULL == pHandleNode)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "pHandleNode is NULL, sequence = %d", _FL_, i);
                continue;
            }

            if((AS_TRUE == pHandleNode->m_bRemoved) ||
                (AS_FALSE != pHandleNode->m_bRemoved))
            {
                continue;
            }
            as_handle *pHandle = pHandleNode->m_pHandle;
            if(NULL == pHandle)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "pHandle is NULL, sequence = %d", _FL_, i);
                continue;
            }

            if(m_epEvents[i].events & EPOLLIN)
            {
                this->checkSelectResult(enEpollRead, pHandle);
            }

            if(m_epEvents[i].events & EPOLLOUT)
            {
                this->checkSelectResult(enEpollWrite, pHandle);
            }
        }

        ListOfHandleIte itListOfHandle = m_listHandle.begin();
        while(itListOfHandle != m_listHandle.end())
        {
            if(AS_TRUE == (*itListOfHandle)->m_bRemoved)
            {
                CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
                    "(*itListOfHandle) removed.", _FL_);

                pHandleNode = *itListOfHandle;
                itListOfHandle = m_listHandle.erase(itListOfHandle);
                AS_DELETE(pHandleNode);
                continue;
            }
            ++itListOfHandle;
        }

        (void)as_mutex_unlock(m_pMutexListOfHandle);
    }
    return;
}
#endif


void as_handle_manager::exit()
{
    if(NULL == m_pSVSThread)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::exit: m_pSVSThread is null", _FL_);
        return;
    }

    this->m_bExit = AS_TRUE;
    errno = 0;
    int32_t ret_val = as_join_thread(m_pSVSThread);
    if (ret_val != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "Wait play thread exit failed. ret_val(%d). error(%d):%s",
            _FL_, ret_val, CONN_ERRNO, strerror(CONN_ERRNO));
    }

    CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
        "as_handle_manager::exit: manager type: %s. exit complete."
        "Thread = %d", _FL_, m_szMgrType, m_pSVSThread->pthead);

    return;
}

int32_t as_handle_manager::addHandle(as_handle *pHandle,
                                  AS_BOOLEAN bIsListOfHandleLocked)
{
    if (NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::addHandle: pHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(InvalidSocket == pHandle->m_lSockFD)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::addHandle: pHandle's socket is invalid", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    as_handle_node *pHandleNode = NULL;
    (void)AS_NEW(pHandleNode);
    if (NULL == pHandleNode )
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::addHandle: new pHandleNode fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    AS_BOOLEAN bNeedLock = AS_FALSE;
    AS_BOOLEAN bLocked = AS_FALSE;
    if(AS_FALSE == bIsListOfHandleLocked)
    {
        if (NULL == m_pSVSThread)
        {
            bNeedLock = AS_TRUE;
        }
        else
        {
            if(as_thread_self() != m_pSVSThread->pthead)
            {
                bNeedLock = AS_TRUE;
            }
        }

        if(AS_TRUE == bNeedLock)
        {
            if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_handle_manager::removeHandle: get lock failed", _FL_);
            }
            else
            {
               bLocked = AS_TRUE;
            }
        }
    }

#if AS_APP_OS == AS_OS_LINUX || AS_APP_OS == AS_OS_ANDROID
    struct epoll_event epEvent;
    memset(&epEvent, 0, sizeof(epEvent));
    epEvent.data.ptr = (void *)pHandleNode;
    epEvent.events = pHandle->getEvents();

    errno = 0;
    if ( 0 != epoll_ctl(m_lEpfd, EPOLL_CTL_ADD, pHandle->m_lSockFD, &epEvent))
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::addHandle: add event fail, "
            "errno = %d, error: %s", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
        AS_DELETE(pHandleNode);

        if(AS_TRUE == bLocked)
        {
            if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "as_handle_manager::addHandle: release lock failed", _FL_);
            }
        }

        return AS_ERROR_CODE_FAIL;
    }
#endif
    pHandle->m_pHandleNode = pHandleNode;

#if AS_APP_OS == AS_OS_LINUX || AS_APP_OS == AS_OS_ANDROID
    pHandle->m_lEpfd = m_lEpfd;
#endif
    pHandleNode->m_pHandle = pHandle;
#if (AS_APP_OS & AS_OS_UNIX) == AS_OS_UNIX
    CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::addHandle: "
            "new pHandleNode(0x%x) m_pHandle(0x%x) fd(%d)"
            "peer_ip(0x%x) peer_port(%d)",
            _FL_, pHandleNode, pHandleNode->m_pHandle,
            pHandleNode->m_pHandle->m_lSockFD,
            pHandleNode->m_pHandle->m_localAddr.m_ulIpAddr,
            pHandleNode->m_pHandle->m_localAddr.m_usPort);
#elif AS_APP_OS == AS_OS_WIN32
    CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
        "as_handle_manager::addHandle: "
        "new pHandleNode(0x%x) m_pHandle(0x%x) fd(%d) "
        "peer_ip(0x%x) peer_port(%d)",
        _FL_, pHandleNode, pHandleNode->m_pHandle,
        pHandleNode->m_pHandle->m_lSockFD,
        pHandleNode->m_pHandle->m_localAddr.m_ulIpAddr,
        pHandleNode->m_pHandle->m_localAddr.m_usPort);
#endif
    m_listHandle.push_back(pHandleNode);

    if(AS_TRUE == bLocked)
    {
        if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_handle_manager::addHandle: release lock failed", _FL_);
        }
    }

    return AS_ERROR_CODE_OK;
}


void as_handle_manager::removeHandle(as_handle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::removeHandle: pHandle is NULL", _FL_);
        return;
    }

    AS_BOOLEAN bNeedLock = AS_FALSE;
    AS_BOOLEAN bLocked = AS_FALSE;
    if (NULL == m_pSVSThread)
    {
        bNeedLock = AS_TRUE;
    }
    else
    {
        if(as_thread_self() != m_pSVSThread->pthead)
        {
            bNeedLock = AS_TRUE;
        }
    }

    if(AS_TRUE == bNeedLock)
    {
        if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_handle_manager::removeHandle: get lock failed", _FL_);
        }
        else
        {
            bLocked = AS_TRUE;
        }
    }

    if(NULL == pHandle->m_pHandleNode)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "pHandle->m_pHandleNode is NULL", _FL_);
    }
    else
    {
        if(pHandle->m_pHandleNode->m_bRemoved != AS_TRUE)
        {
            pHandle->close();
            pHandle->m_pHandleNode->m_bRemoved = AS_TRUE;
            pHandle->m_pHandleNode->m_pHandle = NULL;
        }
        else
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "pHandle removed more than once", _FL_);
        }

        pHandle->m_pHandleNode = NULL;
    }

    if(AS_TRUE == bLocked)
    {
        if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_handle_manager::removeHandle: release lock failed", _FL_);
        }
    }

    return;
}


void as_tcp_conn_mgr::lockListOfHandle()
{
    if (AS_ERROR_CODE_OK != as_mutex_lock(m_pMutexListOfHandle))
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_mgr::lockListOfHandle: get lock failed", _FL_);
    }
}

void as_tcp_conn_mgr::unlockListOfHandle()
{
    if (AS_ERROR_CODE_OK != as_mutex_unlock(m_pMutexListOfHandle))
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_tcp_conn_mgr::unlockListOfHandle: release lock failed", _FL_);
    }
}

void as_tcp_conn_mgr::checkSelectResult(const EpollEventType enEpEvent,
    as_handle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::checkSelectResult: pHandle is NULL", _FL_);
        return;
    }

    as_tcp_conn_handle *pTcpConnHandle = dynamic_cast<as_tcp_conn_handle *>(pHandle);
    if(NULL == pTcpConnHandle)
    {
        return;
    }

    if(enEpollRead == enEpEvent)
    {
        pTcpConnHandle->setHandleRecv(AS_FALSE);
        pTcpConnHandle->handle_recv();
    }

    if(enEpollWrite == enEpEvent)
    {
        pTcpConnHandle->setHandleSend(AS_FALSE);

        if(pTcpConnHandle->getStatus() == enConnecting)
        {
            int32_t lErrorNo = 0;
            socklen_t len = sizeof(lErrorNo);
            if (getsockopt((SOCKET)(pTcpConnHandle->m_lSockFD), SOL_SOCKET, SO_ERROR,
                (SOCK_OPT_TYPE *)&lErrorNo, &len) < 0)
            {
                CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                    "getsockopt of sockfd has wrong", _FL_);
                pTcpConnHandle->close();
                pTcpConnHandle->m_lConnStatus = enConnFailed;
            }
            else if (lErrorNo != 0)
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "getsockopt says sockfd is wrong", _FL_);
                pTcpConnHandle->close();
                pTcpConnHandle->m_lConnStatus = enConnFailed;
            }

            pTcpConnHandle->m_lConnStatus = enConnected;
        }

        pTcpConnHandle->handle_send();
    }
}


void as_udp_sock_mgr::checkSelectResult(const EpollEventType enEpEvent,
    as_handle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::checkSelectResult: pHandle is NULL", _FL_);
        return;
    }

    as_udp_sock_handle *pUdpSockHandle = dynamic_cast<as_udp_sock_handle *>(pHandle);
    if(NULL == pUdpSockHandle)
    {
        return;
    }

    if(enEpollRead == enEpEvent)
    {
        pUdpSockHandle->setHandleRecv(AS_FALSE);
        pUdpSockHandle->handle_recv();
    }

    if(enEpollWrite == enEpEvent)
    {
        pUdpSockHandle->setHandleSend(AS_FALSE);
        pUdpSockHandle->handle_send();
    }
}

void as_tcp_server_mgr::checkSelectResult(const EpollEventType enEpEvent,
    as_handle *pHandle)
{
    if(NULL == pHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_handle_manager::checkSelectResult: pHandle is NULL", _FL_);
        return;
    }

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_server_mgr::checkSelectResult: m_pTcpConnMgr is NULL.", _FL_);
        return;
    }

    as_tcp_server_handle *pTcpServerHandle = dynamic_cast<as_tcp_server_handle *>(pHandle);
    if(NULL == pTcpServerHandle)
    {
        return;
    }

    if(enEpollRead == enEpEvent)
    {
        struct sockaddr_in peerAddr;
        memset(&peerAddr, 0, sizeof(struct sockaddr_in));

        socklen_t len = sizeof(struct sockaddr_in);
        int32_t lClientSockfd = InvalidFd;
        errno = 0;
        lClientSockfd = (int32_t)::accept((SOCKET)(pTcpServerHandle->m_lSockFD),
            (struct sockaddr *)&peerAddr, &len);
        if( 0 > lClientSockfd)
        {
            if((EWOULDBLOCK != CONN_ERRNO) && (CONN_ERRNO != EAGAIN))
            {
                CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                    "accept Error(%d): %s. ", _FL_, CONN_ERRNO, strerror(CONN_ERRNO));
            }
            return;
        }
        //setSendBufSize
        int32_t lSendBufSize = DEFAULT_TCP_SENDRECV_SIZE;
        socklen_t lSendBufLength = sizeof(lSendBufSize);
        if(setsockopt((SOCKET)lClientSockfd, SOL_SOCKET, SO_SNDBUF, (char*)&lSendBufSize,
            lSendBufLength) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "setSendBufSize client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }

        //setRecvBufSize
        int32_t lRecvBufSize = DEFAULT_TCP_SENDRECV_SIZE;
        socklen_t lRecvBufLength = sizeof(lRecvBufSize);
        if(setsockopt((SOCKET)lClientSockfd, SOL_SOCKET, SO_RCVBUF, (char*)&lRecvBufSize,
            lRecvBufLength) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "setRecvBufSize client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }
        int32_t flag = 1;
        if(setsockopt((SOCKET)lClientSockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag,
            sizeof(flag)) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "set TCP_NODELAY client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }
        //setReuseAddr();
        int32_t lReuseAddrFlag = 1;
        if(setsockopt((SOCKET)lClientSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&lReuseAddrFlag,
            sizeof(lReuseAddrFlag)) < 0)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "setsockopt client socket error(%d)", _FL_, CONN_ERRNO);
            return;
        }

        as_network_addr clientAddr;
        clientAddr.m_ulIpAddr = (LONG)peerAddr.sin_addr.s_addr;
        clientAddr.m_usPort = peerAddr.sin_port;
        as_tcp_conn_handle *pTcpConnHandle = NULL;


        m_pTcpConnMgr->lockListOfHandle();
        if (AS_ERROR_CODE_OK != pTcpServerHandle->handle_accept(&clientAddr, pTcpConnHandle))
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_tcp_server_mgr::checkSelectResult: accept fail.", _FL_);
            m_pTcpConnMgr->unlockListOfHandle();
            return;
        }

        if (NULL == pTcpConnHandle)
        {
            (void)CLOSESOCK((SOCKET)lClientSockfd);
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_tcp_server_mgr::checkSelectResult: "
                "return NULL arg.", _FL_);
            m_pTcpConnMgr->unlockListOfHandle();
            return;
        }
        if(AS_ERROR_CODE_OK != pTcpConnHandle->initHandle())
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_tcp_server_mgr::checkSelectResult: "
                "pTcpConnHandle init fail", _FL_);
            m_pTcpConnMgr->unlockListOfHandle();
            return;
        }
        pTcpConnHandle->m_lSockFD = lClientSockfd;
        pTcpConnHandle->m_localAddr.m_ulIpAddr = pTcpServerHandle->m_localAddr.m_ulIpAddr;
        pTcpConnHandle->m_localAddr.m_usPort = pTcpServerHandle->m_localAddr.m_usPort;
        pTcpConnHandle->m_lConnStatus = enConnected;
        pTcpConnHandle->m_peerAddr.m_ulIpAddr = clientAddr.m_ulIpAddr;
        pTcpConnHandle->m_peerAddr.m_usPort = clientAddr.m_usPort;

        AS_BOOLEAN bIsListOfHandleLocked = AS_TRUE;
        if (AS_ERROR_CODE_OK != m_pTcpConnMgr->addHandle(pTcpConnHandle,
                                                    bIsListOfHandleLocked))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_tcp_server_mgr::checkSelectResult: addHandle fail.", _FL_);
            pTcpConnHandle->close();
        }
        m_pTcpConnMgr->unlockListOfHandle();
        CONN_WRITE_LOG(CONN_DEBUG, (char *)"FILE(%s)LINE(%d): "
            "as_tcp_server_mgr::checkSelectResult: accept connect, "
            "m_lSockFD = %d, peer_ip(0x%x), peer_port(%d)", _FL_,
            pTcpConnHandle->m_lSockFD,
            ntohl((ULONG)(pTcpConnHandle->m_peerAddr.m_ulIpAddr)),
            ntohs(pTcpConnHandle->m_peerAddr.m_usPort));

    }


    if(enEpollWrite == enEpEvent)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_tcp_server_mgr should not process write event", _FL_);
    }
}


as_network_svr::as_network_svr()
{
    m_lLocalIpAddr = InvalidIp;
    m_pTcpConnMgr = NULL;
    m_pUdpSockMgr = NULL;
    m_pTcpServerMgr = NULL;

}

as_network_svr::~as_network_svr()
{
    try
    {
        AS_DELETE(m_pTcpConnMgr);
        AS_DELETE(m_pUdpSockMgr);
        AS_DELETE(m_pTcpServerMgr);
    }
    catch (...)
    {
    }
}


int32_t as_network_svr::init(const ULONG ulSelectPeriod, const AS_BOOLEAN bHasUdpSock,
            const AS_BOOLEAN bHasTcpClient, const AS_BOOLEAN bHasTcpServer)
{
#if AS_APP_OS == AS_OS_WIN32
    WSAData wsaData;
    if (SOCKET_ERROR == WSAStartup(MAKEWORD(2,2),&wsaData))
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "WSAStartup error.", _FL_);
        return AS_ERROR_CODE_FAIL;
    }
#endif
    if(AS_TRUE == bHasUdpSock)
    {
        (void)AS_NEW(m_pUdpSockMgr);
        if(NULL == m_pUdpSockMgr)
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::init: create m_pUdpSockMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != m_pUdpSockMgr->init(ulSelectPeriod))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::init: init m_pUdpSockMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if((AS_TRUE == bHasTcpClient) || (AS_TRUE == bHasTcpServer))
    {
        (void)AS_NEW(m_pTcpConnMgr);
        if(NULL == m_pTcpConnMgr)
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::init: create m_pTcpConnMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != m_pTcpConnMgr->init(ulSelectPeriod))
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::init: init m_pTcpConnMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(AS_TRUE == bHasTcpServer)
    {
        (void)AS_NEW(m_pTcpServerMgr);
        if(NULL == m_pTcpServerMgr)
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::init: create m_pTcpServerMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        if(AS_ERROR_CODE_OK != m_pTcpServerMgr->init(ulSelectPeriod))
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::init: init m_pTcpServerMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
        m_pTcpServerMgr->setTcpClientMgr(m_pTcpConnMgr);
    }

    return AS_ERROR_CODE_OK;
}


int32_t as_network_svr::run(void)
{
    if(NULL != m_pUdpSockMgr)
    {
        if(AS_ERROR_CODE_OK != m_pUdpSockMgr->run())
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::run: run m_pUdpSockMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(NULL != m_pTcpConnMgr)
    {
        if(AS_ERROR_CODE_OK != m_pTcpConnMgr->run())
        {
            CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::run: run m_pTcpConnMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    if(NULL != m_pTcpServerMgr)
    {
        if(AS_ERROR_CODE_OK != m_pTcpServerMgr->run())
        {
            CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
                "as_network_svr::run: run m_pTcpServerMgr fail", _FL_);
            return AS_ERROR_CODE_FAIL;
        }
    }

    return AS_ERROR_CODE_OK;

}


void as_network_svr::exit(void)
{
    if(NULL != m_pUdpSockMgr)
    {
        m_pUdpSockMgr->exit();
        AS_DELETE(m_pUdpSockMgr);
    }

    if(NULL != m_pTcpServerMgr)
    {
        m_pTcpServerMgr->exit();
        AS_DELETE(m_pTcpServerMgr);
    }

    if(NULL != m_pTcpConnMgr)
    {
        m_pTcpConnMgr->exit();
        AS_DELETE(m_pTcpConnMgr);
    }

#if AS_APP_OS == AS_OS_WIN32
    (void)WSACleanup();
#endif
    return;
}


void as_network_svr::setDefaultLocalAddr(const char *szLocalIpAddr)
{
    if(szLocalIpAddr != NULL)
    {
        int32_t lLocalIp = (int32_t)inet_addr(szLocalIpAddr);
        if ((ULONG)lLocalIp != InvalidIp)
        {
            m_lLocalIpAddr = (int32_t)inet_addr(szLocalIpAddr);
        }
    }

    return;
}


int32_t as_network_svr::regTcpClient( const as_network_addr *pLocalAddr,
    const as_network_addr *pPeerAddr, as_tcp_conn_handle *pTcpConnHandle,
    const EnumSyncAsync bSyncConn, ULONG ulTimeOut)
{
    if(NULL == pLocalAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: pLocalAddr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pPeerAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: pPeerAddr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pTcpConnHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: pTcpConnHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != pTcpConnHandle->initHandle())
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: pTcpConnHandle init fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: m_pTcpConnMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    as_network_addr localAddr;
    if (InvalidIp == (ULONG)(pLocalAddr->m_ulIpAddr))
    {
        localAddr.m_ulIpAddr = this->m_lLocalIpAddr;
    }
    else
    {
        localAddr.m_ulIpAddr = pLocalAddr->m_ulIpAddr;
    }
    localAddr.m_usPort = pLocalAddr->m_usPort;

    pTcpConnHandle->m_localAddr.m_ulIpAddr = pLocalAddr->m_ulIpAddr;
    pTcpConnHandle->m_localAddr.m_usPort = pLocalAddr->m_usPort;

    int32_t lRetVal = pTcpConnHandle->conn(&localAddr, pPeerAddr, bSyncConn, ulTimeOut);

    if(lRetVal != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: connect peer fail(%s:%d)", _FL_,
            const_cast<as_network_addr *>(pPeerAddr)->get_host_addr(), ntohs(pPeerAddr->m_usPort));
        return lRetVal;
    }

    lRetVal = m_pTcpConnMgr->addHandle(pTcpConnHandle);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        pTcpConnHandle->close();
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: register connection fail", _FL_);
        return lRetVal;
    }

    return AS_ERROR_CODE_OK;
}

int32_t as_network_svr::regTcpMonitorClient(as_tcp_monitor_handle *pTcpConnHandle,int32_t lSockFD)
{
    if(NULL == pTcpConnHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpMonitorClient: pTcpConnHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != pTcpConnHandle->initHandle())
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpMonitorClient: pTcpConnHandle init fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    pTcpConnHandle->setSockFD(lSockFD);

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpMonitorClient: m_pTcpConnMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    int32_t lRetVal = m_pTcpConnMgr->addHandle(pTcpConnHandle);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        pTcpConnHandle->close();
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpMonitorClient: register connection fail", _FL_);
        return lRetVal;
    }

    return AS_ERROR_CODE_OK;
}

void as_network_svr::removeTcpClient(as_tcp_conn_handle *pTcpConnHandle)
{
    if(NULL == pTcpConnHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::removeTcpClient: pTcpConnHandle is NULL", _FL_);
        return;
    }
    CONN_WRITE_LOG(CONN_DEBUG,  (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::removeTcpClient: "
            "remove pTcpConnHandle(0x%x) pHandleNode(0x%x) fd(%d)"
            "m_ulIpAddr(0x%x) m_usPort(%d)",
            _FL_, pTcpConnHandle, pTcpConnHandle->m_pHandleNode,
            pTcpConnHandle->m_lSockFD, pTcpConnHandle->m_localAddr.m_ulIpAddr,
            pTcpConnHandle->m_localAddr.m_usPort);

    //pTcpConnHandle->close();

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::removeTcpClient: m_pTcpConnMgr is NULL", _FL_);
        return;
    }

    m_pTcpConnMgr->removeHandle(pTcpConnHandle);
    return;
}


int32_t as_network_svr::regTcpServer(const as_network_addr *pLocalAddr,
    as_tcp_server_handle *pTcpServerHandle)
{
    if(NULL == pLocalAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpServer: pLocalAddr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pTcpServerHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpServer: pTcpServerHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != pTcpServerHandle->initHandle())
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpServer: pTcpServerHandle init fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pTcpConnMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpServer: m_pTcpConnMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pTcpServerMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpServer: m_pTcpServerMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    as_network_addr localAddr;
    if (InvalidIp == (ULONG)(pLocalAddr->m_ulIpAddr))
    {
        localAddr.m_ulIpAddr = this->m_lLocalIpAddr;
    }
    else
    {
        localAddr.m_ulIpAddr = pLocalAddr->m_ulIpAddr;
    }
    localAddr.m_usPort = pLocalAddr->m_usPort;

    pTcpServerHandle->m_localAddr.m_ulIpAddr = pLocalAddr->m_ulIpAddr;
    pTcpServerHandle->m_localAddr.m_usPort = pLocalAddr->m_usPort;

    int32_t lRetVal = pTcpServerHandle->listen(&localAddr);

    if(lRetVal != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpServer: listen fail", _FL_);
        return lRetVal;
    }

    lRetVal = m_pTcpServerMgr->addHandle(pTcpServerHandle);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        pTcpServerHandle->close();
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regTcpClient: register tcp server fail", _FL_);
        return lRetVal;
    }

    return AS_ERROR_CODE_OK;
}


void as_network_svr::removeTcpServer(as_tcp_server_handle *pTcpServerHandle)
{
    if(NULL == pTcpServerHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::removeTcpServer: pTcpServerHandle is NULL", _FL_);
        return;
    }

    //pTcpServerHandle->close();
    if(NULL == m_pTcpServerMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::removeTcpServer: m_pTcpServerMgr is NULL", _FL_);
        return;
    }

    m_pTcpServerMgr->removeHandle(pTcpServerHandle);
    return;
}


int32_t as_network_svr::regUdpSocket(const as_network_addr *pLocalAddr,
                                 as_udp_sock_handle *pUdpSockHandle,
                                 const as_network_addr *pMultiAddr)
{
    if(NULL == pLocalAddr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regUdpSocket: pUdpSockHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == pUdpSockHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regUdpSocket: pUdpSockHandle is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(AS_ERROR_CODE_OK != pUdpSockHandle->initHandle())
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regUdpSocket: pUdpSockHandle init fail", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    if(NULL == m_pUdpSockMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regUdpSocket: m_pUdpSockMgr is NULL", _FL_);
        return AS_ERROR_CODE_FAIL;
    }

    as_network_addr localAddr;
    if (InvalidIp == (ULONG)(pLocalAddr->m_ulIpAddr))
    {
        localAddr.m_ulIpAddr = this->m_lLocalIpAddr;
    }
    else
    {
        localAddr.m_ulIpAddr = pLocalAddr->m_ulIpAddr;
    }
    localAddr.m_usPort = pLocalAddr->m_usPort;

    pUdpSockHandle->m_localAddr.m_ulIpAddr = pLocalAddr->m_ulIpAddr;
    pUdpSockHandle->m_localAddr.m_usPort = pLocalAddr->m_usPort;

    int32_t lRetVal = pUdpSockHandle->createSock(&localAddr, pMultiAddr);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regUdpSocket: create UDP socket fail", _FL_);
        return lRetVal;
    }

    lRetVal = m_pUdpSockMgr->addHandle(pUdpSockHandle);
    if(lRetVal != AS_ERROR_CODE_OK)
    {
        pUdpSockHandle->close();
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::regUdpSocket: register UDP socket fail", _FL_);
        return lRetVal;
    }

    return AS_ERROR_CODE_OK;
}


void as_network_svr::removeUdpSocket(as_udp_sock_handle *pUdpSockHandle)
{
    if(NULL == pUdpSockHandle)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::removeUdpSocket: pUdpSockHandle is NULL", _FL_);
        return;
    }
    pUdpSockHandle->close();

    if(NULL == m_pUdpSockMgr)
    {
        CONN_WRITE_LOG(CONN_WARNING, (char *)"FILE(%s)LINE(%d): "
            "as_network_svr::removeUdpSocket: m_pUdpSockMgr is NULL", _FL_);
        return;
    }

    m_pUdpSockMgr->removeHandle(pUdpSockHandle);
    return;
}




