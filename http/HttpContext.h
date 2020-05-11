#ifndef CUBER_NET_HTTP_HTTPCONTEXT_H
#define CUBER_NET_HTTP_HTTPCONTEXT_H

#include "base/copyable.h"
#include "http/HttpRequest.h"
#include "HttpTimer.h"

namespace cuber
{
namespace net
{

class Buffer;

class HttpContext : public cuber::copyable
{
public:
  enum HttpRequestParseState
  {
    kExpectRequestLine,
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

  HttpContext() : state_(kExpectRequestLine)
  {
  }

  // default copy-ctor, dtor and assignment are fine

  HttpParseState parseRequest(Buffer *buf, Timestamp receiveTime);

  /*
  * Got All expect HTTP Package
  * */
  bool gotAll() const
  {
    return state_ == kGotAll;
  }

  void reset()
  {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest &request() const
  {
    return request_;
  }

  HttpRequest &request()
  {
    return request_;
  }

  void setTimerPos(HttpTimerPos timerPos)
  { timerPos_ = timerPos; }

  const HttpTimerPos &timerPos()
  { return timerPos_; }

private:
  bool parseRequestLine(const char *begin, const char *end);

  HttpParseState parseHeaders(const char *begin, const char *end);

  bool parseBody(Buffer *buf);

  HttpRequestParseState state_;
  HttpRequest request_;
  HttpTimerPos timerPos_;
};

} // namespace net
} // namespace cuber

#endif // CUBER_NET_HTTP_HTTPCONTEXT_H
