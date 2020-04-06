#include "http/HttpProxyHandler.h"
#include "http/HttpProxy.h"

using namespace cuber;
using namespace cuber::net;

HttpHandleState HttpProxyHandler::handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request,
                                         HttpResponse &response) {
    const std::string &path = request.path();
    std::string location(path.substr(0, request.locationEndPos() + 1));

    if (!config->existProxy(location)) {
        return doHandleNext(config, conn, request, response);
    }

    const ProxyServer &proxy = config->proxyInfo(location);
    const std::string &upstreamName = proxy.upstreamName;
    const std::vector<std::string> &setHeaders = proxy.setHeaders;
    for (const auto &setHeader : setHeaders) {
        if (setHeader == "X-Real-IP") {
            request.addHeader("X-Real-IP", conn->peerAddress().toIp());
            LOG_DEBUG << "Add X-Real-IP: " << request.getHeader("X-Real-IP");
        } else if (setHeader == "X-Forwarded-For") {
            request.addHeader("X-Forwarded-For", conn->peerAddress().toIp());
            LOG_DEBUG << "Add X-Forwarded-For: " << request.getHeader("X-Forwarded-For");
        }
    }

    LOG_DEBUG << "Proxy upstream: " << upstreamName;
    auto upstream = config->upstreamInfo(upstreamName);
    auto addr = upstream.address;
    std::string::size_type colon_pos = addr.find(':');
    std::string ipAddr;
    int port;
    if (colon_pos == std::string::npos) {
        port = 80;
    } else {
        ipAddr.assign(addr.substr(0, colon_pos - 1));
        port = std::stoi(addr.substr(colon_pos + 1));
    }

    InetAddress upstreamAddr(ipAddr, static_cast<uint16_t>(port));
    LOG_DEBUG << "Upstream address: " << upstreamAddr.toIpPort();
    std::string proxyName = conn->name() + "%";
    setConnInfo(conn->name(), conn);

    auto upstreamProxy = new HttpProxy(conn->getLoop(), upstreamAddr, proxyName);
    upstreamProxy->setResponseCallback(std::bind(&HttpProxyHandler::onResponseMessage, this, _1, _2, _3));
    upstreamProxy->setForwardFailedCallback(std::bind(&HttpProxyHandler::onForwardFailed, this, _1, _2));
    upstreamProxy->connect();
    upstreamProxy->forward(request);

    return kForwarded;
}

void HttpProxyHandler::onResponseMessage(const TcpConnectionPtr &responseConn, Buffer *buff, Timestamp recvTime) {
    auto *context = boost::any_cast<HttpResponseContext>(responseConn->getMutableContext());

    switch (context->parseResponse(buff, recvTime)) {
        case HttpResponseContext::kBadMessage:
            onBadUpstreamResponse(context, responseConn);
            break;
        case HttpResponseContext::kParseOK:
            doProxyResponse(context, responseConn, buff);
            break;
        case HttpResponseContext::kParseAgain:
        default:
            break;
    }
}

void HttpProxyHandler::onResponse(const TcpConnectionPtr &requestConn, HttpResponse &resp) {
    messageCallback_(requestConn, resp);
}

void
HttpProxyHandler::doProxyResponse(HttpResponseContext *context, const TcpConnectionPtr &responseConn, Buffer *buff) {
    std::string responseConnName(responseConn->name());
    std::string::size_type split_pos = responseConnName.find_first_of('%');
    assert(split_pos != std::string::npos);
    auto requestConnName = responseConnName.substr(0, split_pos);
    assert(clientConnMap.count(requestConnName));
    TcpConnectionPtr &req_conn = clientConnMap[requestConnName];

    onResponse(req_conn, context->response());

    auto req_context = boost::any_cast<HttpContext>(req_conn->getMutableContext());
    auto req = req_context->request();
    const string &connection = req.getHeader("Connection");
    bool close = connection == "close" ||
                 (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");

    if (close) {
        clientConnMap.erase(requestConnName);
        responseConn->shutdown();
    }
}

void HttpProxyHandler::onBadUpstreamResponse(HttpResponseContext *context, const TcpConnectionPtr &responseConn) {
    std::string responseConnName(responseConn->name());
    std::string::size_type split_pos = responseConnName.find_first_of('%');
    assert(split_pos != std::string::npos);
    auto requestConnName = responseConnName.substr(0, split_pos);
    assert(clientConnMap.count(requestConnName));
    TcpConnectionPtr &req_conn = clientConnMap[requestConnName];

    HttpResponse response;
    response.setStatusCode(HttpResponse::k500InternalServerError);
    onResponse(req_conn, response);

    auto req_context = boost::any_cast<HttpContext>(req_conn->getMutableContext());
    auto req = req_context->request();
    const string &connection = req.getHeader("Connection");
    bool close = connection == "close" ||
                 (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");

    if (close) {
        clientConnMap.erase(requestConnName);
        responseConn->shutdown();
    }
}

void HttpProxyHandler::onForwardFailed(const std::string &clientName, const InetAddress &peerAddr) {
    /* Client name 被赋值为 request connection name */
    std::string::size_type split_pos = clientName.find_first_of('%');
    assert(split_pos != std::string::npos);
    auto requestConnName = clientName.substr(0, split_pos);
    assert(clientConnMap.count(requestConnName));
    TcpConnectionPtr &req_conn = clientConnMap[requestConnName];

    HttpResponse response;
    response.setStatusCode(HttpResponse::k503ServiceUnavailable);
    onResponse(req_conn, response);

    auto req_context = boost::any_cast<HttpContext>(req_conn->getMutableContext());
    auto req = req_context->request();
    const string &connection = req.getHeader("Connection");
    bool close = connection == "close" ||
                 (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");

    if (close) {
        clientConnMap.erase(requestConnName);
    }
}
