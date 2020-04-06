#ifndef CUBER_HTTPPROXYHANDLER_H
#define CUBER_HTTPPROXYHANDLER_H

#include <string>
#include <utility>
#include <unordered_set>
#include "net/TcpConnection.h"
#include "http/HttpHandler.h"


namespace cuber {
namespace net {
class HttpRequest;
class HttpResponse;

/*
 * HttpProxyHandler： proxy 作为中转，收到 Http 请求后，将请求发给 upstream
 * 同时，维护一个 response buffer，收到 response 就解析并放到其中
 * 读取 HTTP response 结束后就返回
 *
 * HTTPProxy 可以做成单例对象，后面可以考虑优化
 *
 * */
typedef std::function<void(const TcpConnectionPtr &, HttpResponse &)> ProxyResponseCallback;

class HttpProxyHandler : public AbstractHttpHandler
{
public:
    explicit HttpProxyHandler() : AbstractHttpHandler()
    {
    }

    explicit HttpProxyHandler(HttpHandlerPtr nextHandler) : AbstractHttpHandler(nextHandler)
    {
    }

    HttpHandleState handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request,
                           HttpResponse &response) override;

    void setResponseCallback(ProxyResponseCallback cb) { messageCallback_ = std::move(cb); }

    void setForwardErrorCallback(ConnectFailCallback cb) { connectFailCallback_ = std::move(cb); }

    void setConnInfo(const std::string &connName, const TcpConnectionPtr &conn)
    {
        clientConnMap[connName] = conn;
    }

private:
    void onResponseMessage(const TcpConnectionPtr &responseConn,
                           Buffer *buff,
                           Timestamp recvTime);

    void onResponse(const TcpConnectionPtr &requestConn,
                    HttpResponse &resp);

    void onBadUpstreamResponse(HttpResponseContext *context, const TcpConnectionPtr &responseConn);

    void onForwardFailed(const std::string &clientName, const InetAddress &peerAddr);

    void doProxyResponse(HttpResponseContext *context, const TcpConnectionPtr &responseConn, Buffer *buff);

    std::map<std::string, TcpConnectionPtr> clientConnMap;
    ProxyResponseCallback messageCallback_;
    ConnectFailCallback connectFailCallback_;
};

} // namespace net
} // namespace cuber

#endif //CUBER_HTTPPROXYHANDLER_H
