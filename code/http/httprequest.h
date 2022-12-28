#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <string>
#include <mysql/mysql.h>
#include <unordered_map>
#include <unordered_set>
#include <errno.h>
#include <regex>
#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/sqlconnRAII.h"

class HttpRequest
{
public:
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,

    };
    enum HTTP_CODE // 状态码
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INIERNAL_ERROR,
        CLOSED_CONNECTION,
    };
    HttpRequest() { Init(); };
    ~HttpRequest() = default;
    void Init();
    bool Parse(Buffer &buffer);
    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string &key) const;
    std::string GetPost(const char *key) const;
    bool IsKeepAlive() const;

private:
    bool m_ParseRequestLine(const std::string &line);
    void m_ParseHeader(const std::string &line);
    void m_ParseBody(const std::string &line);
    void m_ParsePath();
    void m_ParsePost();
    void m_ParseFromUrlencoded();
    static bool m_UserVerify(const std::string &name, const std::string &password, bool IsLogin);
    PARSE_STATE m_state;
    std::string m_method;
    std::string m_path;
    std::string m_version;
    std::string m_body;
    std::unordered_map<std::string, std::string> m_header;
    std::unordered_map<std::string, std::string> m_post;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFALUT_HTML_TAG;
    static int TransformHex(char ch);
};

#endif
