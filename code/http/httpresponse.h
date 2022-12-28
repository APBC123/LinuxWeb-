#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();
    void Init(const std::string &SrcDir, std::string &path, bool IsKeepAlive = false, int code = -1);
    void MakeResponse(Buffer &buffer);
    void UnmapFile();
    char *File();
    size_t FileLen() const;
    void ErrorContent(Buffer &buffer, std::string message);
    int Code() const { return m_code; }
    void AddStateLine(Buffer &buffer);
    void AddHeader(Buffer &buffer);
    void AddContent(Buffer &buffer);
    void ErrorHtml();
    std::string GetFileType();

private:
    int m_code;
    bool m_IsKeepAlive;
    std::string m_path;
    std::string m_SrcDir;
    char *m_File;
    struct stat m_FileStat;
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> CODE_STATUS;
    static const std::unordered_map<int, std::string> CODE_PATH;
};
#endif
