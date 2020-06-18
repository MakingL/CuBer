#include "base/Any.h"
#include "base/FileUtil.h"
#include "base/Logging.h"

#include "http/HttpServer.h"
#include "http/HttpContext.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpProxyHandler.h"
#include "http/HttpTimer.h"

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
          kMaxConnections_(config_->mainConf().maxWorkerConnections),
          keepAliveTimer_(nullptr),
#ifdef PRESSURE_TEST
          httpHandler_(new HttpHandler(new HttpPressureTestHandler)),
#else
        httpHandler_(new HttpHandler(new HttpAccessHandler(new HttpStaticHandler))),
#endif
          httpFilter_(new HttpFilter(new HttpHeadersFilter(new HttpWriteFilter))) {
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(std::bind(&HttpServer::onMessage, this, _1, _2, _3));
    httpHandler_->appendHandler(new HttpPostReadHandler);
    auto proxyHandler = new HttpProxyHandler;
    proxyHandler->setResponseCallback(std::bind(&HttpServer::proxyResponseCallback, this, _1, _2));
    httpHandler_->appendHandler(proxyHandler);
    httpHandler_->appendHandler(new HttpDefaultHandler);
#ifndef PRESSURE_TEST
    if (config_->mainConf().keepAliveTimeout > 0) {
        keepAliveTimer_.reset(new HttpTimer(loop, config_->mainConf().keepAliveTimeout));
    }
#endif
}

void HttpServer::start() {
    LOG_INFO << "HttpServer[" << server_.name()
             << "] starts listening on " << server_.ipPort();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
        LOG_DEBUG << "On connected: " << conn->name();
        if (numConnected_.incrementAndGet() > kMaxConnections_) {
            conn->shutdown();
#ifndef PRESSURE_TEST
            conn->forceCloseWithDelay(3.0); // > round trip of the whole Internet.
#else
            conn->forceClose();
            return;
#endif
        }

        if (keepAliveTimer_) {
            HttpContext context = HttpContext();
            context.setTimerPos(keepAliveTimer_->addHttpConnection(conn));
            conn->setContext(context);
        } else {
            conn->setContext(HttpContext());
        }
    } else if (conn->disconnected()) {
        LOG_DEBUG << "On client closed: " << conn->name();
        conn->forceClose();

        if (keepAliveTimer_) {
            auto *context = any_cast<HttpContext>(conn->getMutableContext());
            keepAliveTimer_->removeHttpConnection(context->timerPos());
        }
        numConnected_.decrement();
    }
}

void HttpServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buf,
                           Timestamp receiveTime) {
    auto *context = any_cast<HttpContext>(conn->getMutableContext());

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
        httpFilter_->filter(config_, conn, req, response);
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
    auto context = any_cast<HttpContext>(requestConn->getMutableContext());

    httpFilter_->doNextFilter(config_, requestConn, context->request(), resp);
    context->reset();
}

