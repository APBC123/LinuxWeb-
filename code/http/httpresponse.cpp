#include "httpresponse.h"

using namespace std;

const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    {".html", "text/html"},
    {"xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    //{".gz", "application/x-gzip"},
    //{".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},

};

const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const unordered_map<int, string> HttpResponse::CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse()
{
    m_code = -1;
    m_path = m_SrcDir = "";
    m_IsKeepAlive = false;
    m_File = nullptr;
    m_FileStat = {0};
};

HttpResponse::~HttpResponse()
{
    UnmapFile();
}

void HttpResponse::Init(const string &SrcDir, string &path, bool IsKeepAlive, int code)
{
    assert(SrcDir != "");
    if (m_File)
    {
        UnmapFile();
    }
    m_code = code;
    IsKeepAlive = IsKeepAlive;
    m_path = path;
    m_SrcDir = SrcDir;
    m_File = nullptr;
    m_FileStat = {0};
}

void HttpResponse::MakeResponse(Buffer &buffer)
{
    /*判断请求资源的文件*/
    if (stat((m_SrcDir + m_path).data(), &m_FileStat) < 0 || S_ISDIR(m_FileStat.st_mode))
    {
        m_code = 404;
    }
    else if (!(m_FileStat.st_mode & S_IROTH))
    {
        m_code = 403;
    }
    else if (m_code == -1)
    {
        m_code = 200;
    }
    ErrorHtml();
    AddStateLine(buffer);
    AddHeader(buffer);
    AddContent(buffer);
}

char *HttpResponse::File()
{
    return m_File;
}

size_t HttpResponse::FileLen() const
{
    return m_FileStat.st_size;
}

void HttpResponse::ErrorHtml()
{
    if (CODE_PATH.count(m_code) == 1)
    {
        m_path = CODE_PATH.find(m_code)->second;
        stat((m_SrcDir + m_path).data(), &m_FileStat);
    }
}

void HttpResponse::AddStateLine(Buffer &buffer)
{
    string status;
    if (CODE_STATUS.count(m_code) == 1)
    {
        status = CODE_STATUS.find(m_code)->second;
    }
    else
    {
        m_code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buffer.Append("HTTP/1.1 " + to_string(m_code) + " " + status + "\r\n");
}

void HttpResponse::AddHeader(Buffer &buffer)
{
    buffer.Append("Connection: ");
    if (m_IsKeepAlive)
    {
        buffer.Append("keep-alive\r\n");
        buffer.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else
    {
        buffer.Append("close\r\n");
    }
    buffer.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent(Buffer &buffer)
{
    int srcFd = open((m_SrcDir + m_path).data(), O_RDONLY);
    if (srcFd < 0)
    {
        ErrorContent(buffer, "File NotFound!");
        return;
    }

    /* 将文件映射到内存提高文件的访问速度
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (m_SrcDir + m_path).data());
    int *mmRet = (int *)mmap(0, m_FileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1)
    {
        ErrorContent(buffer, "File NotFound!");
        return;
    }
    m_File = (char *)mmRet;
    close(srcFd);
    buffer.Append("Content-length: " + to_string(m_FileStat.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile()
{
    if (m_File)
    {
        munmap(m_File, m_FileStat.st_size);
        m_File = nullptr;
    }
}

string HttpResponse::GetFileType()
{
    /* 判断文件类型 */
    string::size_type idx = m_path.find_last_of('.');
    if (idx == string::npos)
    {
        return "text/plain";
    }
    string suffix = m_path.substr(idx);
    if (SUFFIX_TYPE.count(suffix) == 1)
    {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer &buff, string message)
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (CODE_STATUS.count(m_code) == 1)
    {
        status = CODE_STATUS.find(m_code)->second;
    }
    else
    {
        status = "Bad Request";
    }
    body += to_string(m_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}
