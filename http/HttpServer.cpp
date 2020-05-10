#include "base/FileUtil.h"
#include "base/Logging.h"

#include "http/HttpServer.h"
#include "http/HttpContext.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpProxyHandler.h"

using namespace cuber;
using namespace cuber::net;

namespace cuber {
namespace net {
namespace detail {
    void defaultHttpCallback(const HttpRequest &req, HttpResponse *resp) {
    }
}  // namespace detail
}  // namespace net
}  // namespace cuber


HttpServer::HttpServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &name,
                       ServerConfig *config,
                       TcpServer::Option option)
        : server_(loop, listenAddr, name, option),
          httpCallback_(detail::defaultHttpCallback),
          serverName_(name),
          config_(config),
          httpHandler_(new HttpHandler(new HttpAccessHandler(new HttpStaticHandler))),
          httpFilter_(new HttpFilter(new HttpHeadersFilter(new HttpWriteFilter))) {
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, _1, _2, _3));
    httpHandler_->appendHandler(new HttpPostReadHandler);
    auto proxyHandler = new HttpProxyHandler;
    proxyHandler->setResponseCallback(std::bind(&HttpServer::proxyResponseCallback, this, _1, _2));
    httpHandler_->appendHandler(proxyHandler);
    httpHandler_->appendHandler(new HttpDefaultHandler);
}

void HttpServer::start() {
    LOG_INFO << "HttpServer[" << server_.name()
             << "] starts listening on " << server_.ipPort();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn) {
    LOG_TRACE << "On connection: " << conn->name();
    if (conn->connected()) {
        conn->setContext(HttpContext());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp receiveTime) {
    auto *context = boost::any_cast<HttpContext>(conn->getMutableContext());

    switch (context->parseRequest(buf, receiveTime)) {
        case HttpContext::kBadMessage:
            onBadMessage(conn);
            break;
        case HttpContext::kParseOK:
            if (onRequest(conn, context->request())) {
                context->reset();
            }
            break;
        case HttpContext::kParseAgain:
        default:
            break;
    }
}

bool HttpServer::onRequest(const TcpConnectionPtr &conn, HttpRequest &req) {
    HttpResponse response;

    bool quitHandle = false;
    bool forwarded = false;
    while (!quitHandle) {
        switch (httpHandler_->handle(config_, conn, req, response)) {
            case kHandleAgain:
                break;
            case kForwarded:
                forwarded = true;
                quitHandle = true;
                break;
            case kHandleDecline:
            case kHandleError:
            case kHandleDone:
            default:
                forwarded = false;
                quitHandle = true;
                break;
        }
    }
    if (!forwarded) {
        httpCallback_(req, &response);
        httpFilter_->filter(conn, req, response);
        return true;
    } else {
        return false;
    }
}

void HttpServer::onBadMessage(const TcpConnectionPtr &conn) {
    HttpResponse response(true);
    response.setStatusCode(HttpResponse::k400BadRequest);
    response.addHeader("Server", serverName_);
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    conn->shutdown();
}


void HttpServer::proxyResponseCallback(const TcpConnectionPtr &requestConn, HttpResponse &resp) {
    auto context = boost::any_cast<HttpContext>(requestConn->getMutableContext());

    httpFilter_->doNextFilter(requestConn, context->request(), resp);
}
