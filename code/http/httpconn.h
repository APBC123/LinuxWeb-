#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>

#include "../log/log.h"
#include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();
    void Init(int SockFd, const sockaddr_in &addr);
    ssize_t Read(int *SaveErrno);
    ssize_t Write(int *SaveErrno);
    void Close();
    int GetFd() const;
    int GetPort() const;
    const char *GetIP() const;
    sockaddr_in GetAddr() const;
    bool Process();
    int ToWriteBytes()
    {
        return m_iov[0].iov_len + m_iov[1].iov_len;
    }

    bool IsKeepAlive() const
    {
        return m_Request.IsKeepAlive();
    }

    static bool IsET;
    static const char *SrcDir;
    static std::atomic<int> UserCount;

private:
    int m_fd;
    struct sockaddr_in m_addr;

    bool m_IsClose;

    int m_iovCnt;
    struct iovec m_iov[2];

    Buffer m_ReadBuffer;  // 读缓冲区
    Buffer m_WriteBuffer; // 写缓冲区

    HttpRequest m_Request;
    HttpResponse m_Response;
};

#endif
