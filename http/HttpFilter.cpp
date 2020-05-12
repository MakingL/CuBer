#include "http/HttpFilter.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "net/TcpConnection.h"
#include "base/Logging.h"

using namespace cuber;
using namespace cuber::net;

HttpFilterState
HttpHeadersFilter::filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &req, HttpResponse &response) {
    LOG_TRACE << "Filter response headers";
    response.addHeader("Server", "Cuber");
    const string &connection = req.getHeader("Connection");
    bool close = connection == "close" ||
                 (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
    response.setCloseConnection(close);
    response.addHeader("Connection", connection);
    if (!close) {
        int timeout = 5;
        if (config->mainConf().keepAliveTimeout > 10) {
            timeout = config->mainConf().keepAliveTimeout - 5;
        }
        response.addHeader("Keep-Alive", std::string("timeout=") + std::to_string(timeout));
    }

    return doNextFilter(config, conn, req, response);
}

HttpFilterState
HttpWriteFilter::filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &req, HttpResponse &response) {
    LOG_TRACE << "Filter response write";
    Buffer buf;
    response.appendToBuffer(&buf);
    conn->send(&buf);
    if (response.closeConnection()) {
        conn->shutdown();
    }

    return doNextFilter(config, conn, req, response);
}