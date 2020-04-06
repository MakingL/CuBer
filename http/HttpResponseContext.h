#ifndef CUBER_HTTPRESPONSECONTEXT_H
#define CUBER_HTTPRESPONSECONTEXT_H

#include "base/Timestamp.h"
#include "base/copyable.h"
#include "http/HttpResponse.h"

namespace cuber {
namespace net {

class Buffer;

class HttpResponseContext : public cuber::copyable
{
public:
    enum HttpResponseParseState
    {
        kExpectStatusLine,
        kExpectHeaders,
        kExpectBody,
        kGotAll,
    };

    enum HttpParseState
    {
        kParseOK,
        kBadMessage,
        kParseAgain,
        kHeaderEnd,
    };

    HttpResponseContext() : state_(kExpectStatusLine)
    {
    }

    HttpParseState parseResponse(Buffer *buff, Timestamp receiveTime);

    bool gotAll() const
    {
        return state_ == kGotAll;
    }

    void reset()
    {
        state_ = kExpectStatusLine;
        HttpResponse dummy;
        response_.swap(dummy);
    }

    const HttpResponse &response() const
    {
        return response_;
    }

    HttpResponse &response()
    {
        return response_;
    }

private:
    HttpParseState parseStatusLine(const char *begin, const char *end);

    HttpParseState parseHeaders(const char *begin, const char *end);

    HttpParseState parseBody(Buffer *buf);

    HttpResponseParseState state_;
    HttpResponse response_;
};

} // namespace net
} // namespace cuber

#endif //CUBER_HTTPRESPONSECONTEXT_H
