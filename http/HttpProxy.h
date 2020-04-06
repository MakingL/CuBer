#ifndef CUBER_HTTPPROXY_H
#define CUBER_HTTPPROXY_H

#include <utility>
#include "config/ServerConfig.h"
#include "net/Buffer.h"
#include "net/TcpClient.h"
#include "http/HttpContext.h"
#include "http/HttpResponseContext.h"

namespace cuber {
namespace net {
class EventLoop;

class HttpProxy : public noncopyable
{
public:
    explicit HttpProxy(EventLoop *loop, InetAddress upstreamAddr, std::string &nameArg);

    ~HttpProxy() = default;

    InetAddress upstreamAddr() { return upstreamAddr_; }

    void forward(HttpRequest &request)
    {
        request.appendToBuffer(&forwardBuff_);

        auto conn = client_.connection();
        if (conn && conn->connected())
        {
            conn->send(&forwardBuff_);
        }
    }

    void connect()
    {
        client_.connect();
    }

    void setForwardFailedCallback(TCPClientConnectFailCallback cb)
    {
        connectFailCallback_ = std::move(cb);
        client_.setConnectFailCallback(connectFailCallback_);
    }

    void setResponseCallback(MessageCallback cb)
    {
        messageCallback_ = std::move(cb);
        client_.setMessageCallback(messageCallback_);
    }

private:
    void onUpstreamConnected(const TcpConnectionPtr &conn);

    Buffer forwardBuff_;
    EventLoop *loop_; // the server acceptor loop
    InetAddress upstreamAddr_;
    TcpClient client_;
    TCPClientConnectFailCallback connectFailCallback_;
    MessageCallback messageCallback_;
};
} // namespace net
} // namespace cuber

#endif //CUBER_HTTPPROXY_H
