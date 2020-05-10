#include "base/Logging.h"
#include "http/HttpProxy.h"

using namespace cuber;
using namespace cuber::net;

HttpProxy::HttpProxy(EventLoop *loop, InetAddress serverAddr, std::string &nameArg)
        : loop_(loop), upstreamAddr_(serverAddr),
          client_(loop, serverAddr, nameArg, 0),
          connectFailCallback_(defaultTCPClientConnectFailCallback),
          messageCallback_(defaultMessageCallback) {

    client_.setConnectionCallback(
            std::bind(&HttpProxy::onUpstreamConnected, this, _1));
    client_.setMessageCallback(messageCallback_);
    client_.setConnectFailCallback(connectFailCallback_);
}

void HttpProxy::onUpstreamConnected(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
        conn->setContext(HttpResponseContext());
        if (forwardBuff_.readableBytes()) {
            LOG_TRACE << "Send to: " << conn->name() << " " << forwardBuff_.peek();
            conn->send(&forwardBuff_);
        }
    }
}
