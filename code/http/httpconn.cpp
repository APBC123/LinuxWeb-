#include "httpconn.h"
using namespace std;

HttpConn::HttpConn()
{
    m_fd = -1;
    m_addr = {0};
    m_IsClose = true;
}

HttpConn::~HttpConn()
{
    Close();
}

void HttpConn::Init(int fd, const sockaddr_in &addr)
{
    assert(fd > 0);
    UserCount++;
    m_addr = addr;
    m_fd = fd;
    m_WriteBuffer.RetrieveAll();
    m_ReadBuffer.RetrieveAll();
    m_IsClose = false;
    LOG_INFO("Client[%d](%s:%d) in, UserCount:%d", m_fd, GetIP(), GetPort(), (int)UserCount);
}

void HttpConn::Close()
{
    m_Response.UnmapFile();
    if (m_IsClose == false)
    {
        m_IsClose = true;
        UserCount--;
        close(m_fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", m_fd, GetIP(), GetPort(), (int)UserCount);
    }
}

int HttpConn::GetFd() const
{
    return m_fd;
}

struct sockaddr_in HttpConn::GetAddr() const
{
    return m_addr;
}

const char *HttpConn::GetIP() const
{
    return inet_ntoa(m_addr.sin_addr);
}

int HttpConn::GetPort() const
{
    return m_addr.sin_port;
}

ssize_t HttpConn::Read(int *SaveErrno)
{
    ssize_t len = -1;
    do
    {
        len = m_ReadBuffer.ReadFd(m_fd, SaveErrno);
        if (len <= 0)
        {
            break;
        }
    } while (IsET);
    return len;
}

ssize_t HttpConn::Write(int *SaveErrno)
{
    ssize_t len = -1;
    do
    {
        len = writev(m_fd, m_iov, m_iovCnt);
        if (len <= 0)
        {
            *SaveErrno = errno;
            break;
        }
        if (m_iov[0].iov_len + m_iov[1].iov_len == 0)
        {
            break;
        } /*传输结束*/
        else if (static_cast<size_t>(len) > m_iov[0].iov_len)
        {
            m_iov[1].iov_base = (uint8_t *)m_iov[1].iov_base + (len - m_iov[0].iov_len);
            m_iov[1].iov_len -= (len - m_iov[0].iov_len);
            if (m_iov[0].iov_len)
            {
                m_WriteBuffer.RetrieveAll();
                m_iov[0].iov_len = 0;
            }
        }
        else
        {
            m_iov[0].iov_base = (uint8_t *)m_iov[0].iov_base + len;
            m_iov[0].iov_len -= len;
            m_WriteBuffer.Retrieve(len);
        }
    } while (IsET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::Process()
{
    m_Request.Init();
    if (m_ReadBuffer.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (m_Request.Parse(m_ReadBuffer))
    {
        LOG_DEBUG("%s", m_Request.path().c_str());
        m_Response.Init(SrcDir, m_Request.path(), m_Request.IsKeepAlive(), 200);
    }
    else
    {
        m_Response.Init(SrcDir, m_Request.path(), false, 400);
    }
    m_Response.MakeResponse(m_WriteBuffer);
    /* 响应头 */
    m_iov[0].iov_base = const_cast<char *>(m_WriteBuffer.Peek());
    m_iov[0].iov_len = m_WriteBuffer.ReadableBytes();
    m_iovCnt = 1;

    /* 文件 */
    if (m_Response.FileLen() > 0 && m_Response.File())
    {
        m_iov[1].iov_base = m_Response.File();
        m_iov[1].iov_len = m_Response.FileLen();
        m_iovCnt = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", m_Response.FileLen(), m_iovCnt, ToWriteBytes());
    return true;
}
