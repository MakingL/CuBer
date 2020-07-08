#ifndef CUBER_NET_HTTP_HTTPSERVER_H
#define CUBER_NET_HTTP_HTTPSERVER_H

#include "net/TcpServer.h"
#include "config/ServerConfig.h"
#include "http/HttpHandler.h"
#include "http/HttpFilter.h"

using namespace cuber::config;

namespace cuber {
namespace net{

class HttpRequest;
class HttpResponse;
class HttpTimer;

class HttpServer : noncopyable
{
public:
    typedef std::function<void(const HttpRequest &,
                               HttpResponse *)>
        HttpCallback;

    HttpServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &name,
               ServerConfig *config,
               TcpServer::Option option = TcpServer::kNoReusePort);

    EventLoop *getLoop() const { return server_.getLoop(); }

    /// Not thread safe, callback be registered before calling start().
    void setHttpCallback(const HttpCallback &cb)
    {
        httpCallback_ = cb;
    }

    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    void start();

    void proxyResponseCallback(const TcpConnectionPtr &requestConn,
                               HttpResponse &resp);

private:
    void onConnection(const TcpConnectionPtr &conn);

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buf,
                   Timestamp receiveTime);

    bool onRequest(const TcpConnectionPtr &, HttpRequest &);

    void onBadMessage(const TcpConnectionPtr &conn);

    TcpServer server_;
    HttpCallback httpCallback_;
    const string serverName_;
    ServerConfig *config_;
    const int kMaxConnections_;
    AtomicInt32 numConnected_;
    std::shared_ptr<HttpTimer> keepAliveTimer_;
    std::unique_ptr<AbstractHttpHandler> httpHandler_;
    std::unique_ptr<AbstractHttpFilter> httpFilter_;
};

typedef std::shared_ptr<cuber::net::HttpServer> HttpServerPtr;

} // namespace net
} // namespace cuber

#endif // CUBER_NET_HTTP_HTTPSERVER_H
