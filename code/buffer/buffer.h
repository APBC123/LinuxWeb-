#ifndef BUFFER_H
#define BUFFER_H

#include <cstring> //perror
#include <iostream>
#include <assert.h>
#include <sys/uio.h>
#include <atomic>
#include <unistd.h>
#include <vector>

class Buffer
{
public:
    Buffer(int InitBufferSize = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;

    const char *Peek() const;
    const char *BeginWriteConst() const;
    char *BeginWrite();
    void HasWritten(size_t len);
    void Retrieve(size_t len);
    void RetrieveAll();             // 用于清空缓冲区
    std::string RetrieveAllToStr(); // 将缓冲区数据以字符串形式返回，随后清空缓冲区数据
    void RetrieveUntil(const char *end);
    void Append(const std::string &str);
    void Append(const char *str, size_t len);
    // void Append(const void *data, size_t len);
    void Append(const Buffer &buffer);
    ssize_t ReadFd(int fd, int *Errno);
    ssize_t WriteFd(int fd, int *Errno);

private:
    void m_MakeSpace(size_t len);
    std::vector<char> m_buffer;
    std::atomic<std::size_t> m_ReadPos;
    std::atomic<std::size_t> m_WritePos;
};
#endif
