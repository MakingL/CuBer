#ifndef CUBER_HTTPHANDLER_H
#define CUBER_HTTPHANDLER_H

#include <string>
#include <utility>
#include <unordered_set>
#include "config/ServerConfig.h"
#include "base/Logging.h"
#include "base/noncopyable.h"
#include "http/HttpContext.h"
#include "http/HttpResponseContext.h"

using namespace cuber::config;

namespace cuber {
namespace net {

class HttpRequest;
class HttpResponse;

enum HttpHandleState
{
    kHandleDone,
    kHandleError,
    kHandleAgain,
    kHandleDecline,
    kForwarded,
};

class AbstractHttpHandler : cuber::noncopyable
{
public:
    explicit AbstractHttpHandler() : nextHandler_(nullptr)
    {
    }

    explicit AbstractHttpHandler(AbstractHttpHandler *nextHandler) : nextHandler_(nextHandler)
    {
    }

    virtual ~AbstractHttpHandler() = default;

    void setNextHandler(AbstractHttpHandler *nextHandler)
    {
        nextHandler_.reset(nextHandler);
    }

    void appendHandler(AbstractHttpHandler *nextHandler)
    {
        auto handler = nextHandler_.get();
        while (handler->nextHandler_.get())
        {
            handler = handler->nextHandler_.get();
        }
        handler->setNextHandler(nextHandler);
    }

    virtual HttpHandleState
    handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request, HttpResponse &response) = 0;

    HttpHandleState doHandleNext(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request, HttpResponse &response)
    {
        if (!nextHandler_)
            return kHandleDone;
        return nextHandler_->handle(config, conn, request, response);
    }

private:
    std::unique_ptr<AbstractHttpHandler> nextHandler_;
};

typedef AbstractHttpHandler *HttpHandlerPtr;

/*
 * 作为第一个 Handler 被调用 
 */
class HttpHandler : public AbstractHttpHandler
{
public:
    explicit HttpHandler() : AbstractHttpHandler()
    {
    }

    explicit HttpHandler(HttpHandlerPtr nextHandler) : AbstractHttpHandler(nextHandler)
    {
    }

    HttpHandleState handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request, HttpResponse &response) override
    {
        assert(config);
        return doHandleNext(config, conn, request, response);
    }
};

/*
* HttpAccessHandler:
*      得到 uri，然后在 Accessible location 字典上查找是否有符合条件的 location
*      如果有代理的 upstream，则可以通过，
*      否则，尝试重写 uri，看是否存在静态文件，
*      否则，返回拒绝访问 kHandleDecline
* */
class HttpAccessHandler : public AbstractHttpHandler
{
public:
    explicit HttpAccessHandler() : AbstractHttpHandler()
    {
    }

    explicit HttpAccessHandler(HttpHandlerPtr nextHandler) : AbstractHttpHandler(nextHandler)
    {
    }

    HttpHandleState handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request, HttpResponse &response) override;
};

class HttpPostReadHandler : public AbstractHttpHandler
{
public:
    explicit HttpPostReadHandler() : AbstractHttpHandler()
    {
    }

    explicit HttpPostReadHandler(HttpHandlerPtr nextHandler) : AbstractHttpHandler(nextHandler)
    {
    }

    HttpHandleState handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request, HttpResponse &response) override;
};

class HttpStaticHandler : public AbstractHttpHandler
{
public:
    explicit HttpStaticHandler() : AbstractHttpHandler()
    {
    }

    explicit HttpStaticHandler(HttpHandlerPtr nextHandler) : AbstractHttpHandler(nextHandler)
    {
    }

    HttpHandleState handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request, HttpResponse &response) override;
};

class HttpDefaultHandler : public AbstractHttpHandler
{
public:
    explicit HttpDefaultHandler() : AbstractHttpHandler()
    {
    }

    explicit HttpDefaultHandler(HttpHandlerPtr nextHandler) : AbstractHttpHandler(nextHandler)
    {
    }

    HttpHandleState handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request, HttpResponse &response) override;
};

} // namespace net
} // namespace cuber

#endif //CUBER_HTTPHANDLER_H
