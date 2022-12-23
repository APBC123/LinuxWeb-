#include "buffer.h"

Buffer::Buffer(int InitBufferSize = 1024) : m_buffer(InitBufferSize)
{
    m_ReadPos = 0;
    m_WritePos = 0;
}

size_t Buffer::WritableBytes() const
{
    return m_buffer.size() - m_WritePos;
}
size_t Buffer::ReadableBytes() const
{
    return m_buffer.size() - m_ReadPos;
}

const char *Buffer::Peek() const
{
    return &*m_buffer.begin() + m_ReadPos;
}

const char *Buffer::BeginWriteConst() const
{
    return &*m_buffer.begin() + m_WritePos;
}

char *Buffer::BeginWrite()
{
    return &*m_buffer.begin() + m_WritePos;
}

void Buffer::HasWritten(size_t len)
{
    m_WritePos += len;
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    m_ReadPos += len;
}

void Buffer::RetrieveAll()
{
    bzero(&m_buffer[0], m_buffer.size());
    m_ReadPos = 0;
    m_WritePos = 0;
}

std::string Buffer::RetrieveAllToStr()
{
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    if (WritableBytes() < len)
    {
        m_MakeSpace(len);
    }
    assert(WritableBytes() >= len);
    std::copy(str, str + len, &*m_buffer.begin() + m_WritePos);
    HasWritten(len);
}

void Buffer::Append(const std::string &str)
{
    Append(str.data(), str.length());
}

void Buffer::Append(const Buffer &buffer)
{
    Append(buffer.Peek(), buffer.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int *Errno)
{
    char buffer[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    /*分散读，保证数据读完*/
    iov[0].iov_base = &*m_buffer.begin() + m_WritePos;
    iov[0].iov_len = writable;
    iov[1].iov_base = buffer;
    iov[1].iov_len = sizeof(buffer);
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        *Errno = errno;
    }
    else if (static_cast<size_t>(len) <= writable)
    {
        m_WritePos += len;
    }
    else
    {
        m_WritePos = m_buffer.size();
        Append(buffer, len - writable);
    }
    return len;
}
ssize_t Buffer::WriteFd(int fd, int *Errno) // Errno为错误码
{
    size_t ReadSize = ReadableBytes();
    ssize_t len = write(fd, Peek(), ReadSize);
    if (len < 0)
    {
        *Errno = errno;
        return len;
    }
    m_ReadPos += len;
    return len;
}

void Buffer::m_MakeSpace(size_t len)
{
    if (m_ReadPos + WritableBytes() < len)
    {
        m_buffer.resize(m_WritePos + len + 1);
    }
    else
    {
        size_t readable = ReadableBytes();
        std::copy(&*m_buffer.begin() + m_ReadPos, &*m_buffer.begin() + m_WritePos, &*m_buffer.begin());
        m_ReadPos = 0;
        m_WritePos = m_ReadPos + readable;
        assert(readable == ReadableBytes());
    }
}

void Buffer::RetrieveUntil(const char *end)
{
    assert(Peek() <= end);
    Retrieve(end - Peek());
}
