#include "httprequest.h"
using namespace std;

const unordered_set<string> HttpRequest::DEFAULT_HTML{
    "/index",
    "/register",
    "/login",
    "/welcome",
    "/video",
    "/picture",
};

const unordered_map<string, int> HttpRequest::DEFALUT_HTML_TAG{
    {"/register.html", 0},
    {"/login.html", 1},
};

void HttpRequest::Init()
{
    m_method = " ";
    m_path = " ";
    m_version = " ";
    m_body = " ";
    m_state = REQUEST_LINE;
    m_header.clear();
    m_post.clear();
}

bool HttpRequest::IsKeepAlive() const
{
    if (m_header.count("Connection") == 1)
    {
        return m_header.find("Connection")->second == "keep-alive" && m_version == "1.1";
    }
    return false;
}

bool HttpRequest::Parse(Buffer &buffer)
{
    const char CRLF[] = "\r\n";
    if (buffer.ReadableBytes() <= 0)
    {
        return false;
    }
    while (buffer.ReadableBytes() && m_state != FINISH)
    {
        const char *LineEnd = search(buffer.Peek(), buffer.BeginWriteConst(), CRLF, CRLF + 2);
        string Line(buffer.Peek(), LineEnd);
        switch (m_state)
        {
        case REQUEST_LINE:
            if (!m_ParseRequestLine(Line))
            {
                return false;
            }
            m_ParsePath();
            break;
        case HEADERS:
            m_ParseHeader(Line);
            if (buffer.ReadableBytes() <= 2)
            {
                m_state = FINISH;
            }
            break;
        case BODY:
            m_ParseBody(Line);
            break;
        default:
            break;
        }
        if (LineEnd == buffer.BeginWrite())
        {
            break;
        }
        buffer.RetrieveUntil(LineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", m_method.c_str(), m_path.c_str(), m_version.c_str());
    return true;
}

void HttpRequest::m_ParsePath()
{
    if (m_path == "/")
    {
        m_path = "/index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML)
        {
            if (item == m_path)
            {
                m_path += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::m_ParseRequestLine(const string &line)
{
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    smatch SubMatch;
    if (regex_match(line, SubMatch, patten))
    {
        m_method = SubMatch[1];
        m_path = SubMatch[2];
        m_version = SubMatch[3];
        m_state = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::m_ParseHeader(const string &line)
{
    regex patten("^([^:]*): ?(.*)$");
    smatch SubMatch;
    if (regex_match(line, SubMatch, patten))
    {
        m_header[SubMatch[1]] = SubMatch[2];
    }
    else
    {
        m_state = BODY;
    }
}

void HttpRequest::m_ParseBody(const string &line)
{
    m_body = line;
    m_ParsePost();
    m_state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

int HttpRequest::TransformHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return ch;
}

void HttpRequest::m_ParsePost()
{
    if (m_method == "POST" && m_header["Content-Type"] == "application/x-www-form-urlencoded")
    {
        m_ParseFromUrlencoded();
        if (DEFALUT_HTML_TAG.count(m_path))
        {
            int tag = DEFALUT_HTML_TAG.find(m_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1)
            {
                bool IsLogin;
                if (tag == 1)
                    IsLogin = true;
                else
                    IsLogin = false;
                if (m_UserVerify(m_post["username"], m_post["password"], IsLogin))
                {
                    m_path = "/welcome.html";
                }
                else
                {
                    m_path = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::m_ParseFromUrlencoded()
{
    if (m_body.size() == 0)
        return;
    string key, value;
    int num = 0;
    int i = 0, j = 0;
    int n = m_body.size();
    for (; i < n; i++)
    {
        char ch = m_body[i];
        switch (ch)
        {
        case '=':
            key = m_body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            m_body[i] = ' ';
            break;
        case '%':
            num = TransformHex(m_body[i + 1]) * 16 + TransformHex(m_body[i + 2]);
            m_body[i + 2] = num % 10 + '0';
            m_body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = m_body.substr(j, i - j);
            j = i + 1;
            m_post[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if (m_post.count(key) == 0 && j < i)
    {
        value = m_body.substr(j, i - j);
        m_post[key] = value;
    }
}

bool HttpRequest::m_UserVerify(const string &name, const string &password, bool IsLogin)
{
    if (name == "" || password == " ")
    {
        return false;
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), password.c_str());
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);
    bool flag = false;
    unsigned int j = 0;
    char order[256] = {0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!IsLogin)
        flag = true;
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);
    if (mysql_query(sql, order))
    {
        mysql_free_result(res);
        return false;
    }
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);
    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        /* 注册行为 且 用户名未被使用*/
        if (IsLogin)
        {
            if (password == password)
            {
                flag = true;
            }
            else
            {
                flag = false;
                LOG_DEBUG("password error!");
            }
        }
        else
        {
            flag = false;
            LOG_DEBUG("user used!");
        }
    }
    mysql_free_result(res);
    /* 注册行为 且 用户名未被使用*/
    if (!IsLogin && flag == true)
    {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), password.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order))
        {
            LOG_DEBUG("Insert error!");
            flag = false;
        }
        flag = true;
    }
    SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG("UserVerify success!!");
    return flag;
}

string HttpRequest::path() const
{
    return m_path;
}

string &HttpRequest::path()
{
    return m_path;
}

string HttpRequest::method() const
{
    return m_method;
}

string HttpRequest::version() const
{
    return m_version;
}

string HttpRequest::GetPost(const string &key) const
{
    assert(key != "");
    if (m_post.count(key) == 1)
    {
        return m_post.find(key)->second;
    }
    return "";
}

string HttpRequest::GetPost(const char *key) const
{
    assert(key != nullptr);
    if (m_post.count(key) == 1)
    {
        return m_post.find(key)->second;
    }
    return "";
}
