#ifndef CUBER_NET_HTTP_HTTPRESPONSE_H
#define CUBER_NET_HTTP_HTTPRESPONSE_H

#include "base/copyable.h"
#include "base/Types.h"

#include <unordered_map>

namespace cuber {
namespace net {

class Buffer;

class MimeType
{
private:
    static void init();
    static std::unordered_map<std::string, std::string> mime_;
    MimeType() = default;
    MimeType(const MimeType &m);

public:
    static std::string getMime(const std::string &suffix);

private:
    static pthread_once_t once_control;
};

class HttpResponse : public cuber::copyable
{
public:
    enum HttpStatusCode
    {
        kUnknown = 0,
        k200Ok = 200,
        k202NoContent = 202,
        k206PartialContent = 206,
        k301MovedPermanently = 301,
        k302Found = 302,
        k303SeeOther = 303,
        k304NotModified = 304,
        k307TemporaryRedirect = 307,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k500InternalServerError = 500,
        k502BadGateway = 502,
        k503ServiceUnavailable = 503,
    };
    using HttpStatusMessages = std::unordered_map<int, string>;
    using HttpErrorBody = std::unordered_map<int, string>;

    explicit HttpResponse(bool close = false);

    void setStatusCode(HttpStatusCode code)
    {
        statusCode_ = code;
        if (statusMessageMap_.count(code))
        {
            setStatusMessage(statusMessageMap_[code]);
        }
        if (bodyMessageMap_.count(statusCode_))
        {
            setBody(bodyMessageMap_[statusCode_]);
        }
    }

    int statusCode()
    {
        return statusCode_;
    }

    void setStatusMessage(const string &message) { statusMessage_ = message; }

    void setStatusMessage(const char *start, const char *end)
    {
        statusMessage_ = std::string(start, end);
    }

    std::string statusMessage()
    {
        return statusMessage_;
    }

    void setVersion(const string &version) { version_ = version; }

    void setVersion(const char *start, const char *end)
    {
        version_ = std::string(start, end);
    }

    void setCloseConnection(bool on) { closeConnection_ = on; }

    bool closeConnection() const { return closeConnection_; }

    void setContentType(const string &contentType)
    {
        addHeader("Content-Type", contentType);
    }

    // FIXME: replace string with StringPiece
    void addHeader(const string &key, const string &value) { headers_[key] = value; }

    void addHeader(const char *start, const char *colon, const char *end);

    string getHeader(const string &field);

    void setBody(const string &body) { body_ = body; }

    void setBody(const char *start, const char *end)
    {
        body_ = std::string(start, end);
    }

    void appendToBuffer(Buffer *output) const;

    void swap(HttpResponse &that)
    {
        std::swap(headers_, that.headers_);
        std::swap(statusCode_, that.statusCode_);
        version_.swap(that.version_);
        statusMessage_.swap(that.statusMessage_);
        std::swap(closeConnection_, that.closeConnection_);
        body_.swap(that.body_);
        std::swap(statusMessageMap_, that.statusMessageMap_);
        std::swap(bodyMessageMap_, that.bodyMessageMap_);
    }

    bool isError()
    {
        return statusCode_ >= HttpStatusCode::k500InternalServerError;
    }

    void setResponseSize(int sz)
    {
        responseSize_ = sz;
    }

    int responseSize()
    {
        return responseSize_;
    }

    void setErrorMsg(const string& msg)
    {
        serverErrorMsg_ = msg;
    }

    string errorMsg()
    {
        return serverErrorMsg_;
    }

private:
    std::unordered_map<string, string> headers_;
    HttpStatusCode statusCode_;
    string version_;
    string statusMessage_;
    bool closeConnection_;
    string body_;
    int responseSize_;
    string serverErrorMsg_;

    /* HTTP Status to Message map */
    HttpStatusMessages statusMessageMap_;
    /* Default error message Body */
    HttpErrorBody bodyMessageMap_;
};

} // namespace net
} // namespace cuber

#endif // CUBER_NET_HTTP_HTTPRESPONSE_H
