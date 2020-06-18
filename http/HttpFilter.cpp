#include "http/HttpFilter.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "net/TcpConnection.h"
#include "base/Logging.h"
#include "base/FileLogging.h"

using namespace cuber;
using namespace cuber::net;

pthread_once_t HttpLoggingFilter::accessPonce_ = PTHREAD_ONCE_INIT;
pthread_once_t HttpLoggingFilter::errorPonce_ = PTHREAD_ONCE_INIT;
FileLoggerPtr HttpLoggingFilter::HTTPAccessLogger = nullptr;
FileLoggerPtr HttpLoggingFilter::HTTPErrorLogger = nullptr;

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
    response.setResponseSize(buf.readableBytes());
    conn->send(&buf);
    if (response.closeConnection()) {
        conn->shutdown();
    }

    return doNextFilter(config, conn, req, response);
}

void HttpLoggingFilter::getAccessLogger() {
    HttpLoggingFilter::HTTPAccessLogger = FileLogging::getFileLogger("access");
}

void HttpLoggingFilter::getErrorLogger() {
    HttpLoggingFilter::HTTPErrorLogger = FileLogging::getFileLogger("error");
}

HttpFilterState
HttpLoggingFilter::filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &request,
                          HttpResponse &response) {
    if (!config->configuredLogger()) {
        return doNextFilter(config, conn, request, response);
    }

    LOG_TRACE << "Filter logging request";
    if (config->existLogger("access")) {
        pthread_once(&accessPonce_, &HttpLoggingFilter::getAccessLogger);

        LogStream stream;
        stream << conn->peerAddress().toIp() << " [" << FileLogger::formatTime(Timestamp::now())
               << "] \"" << request.reqLineToString() << "\" " << response.statusCode()
               << " " << response.responseSize() << " \""
               << request.getHeader("User-Agent") << "\"\n";
        HTTPAccessLogger->info(stream);
    }

    if (response.isError() && config->existLogger("error")) {
        pthread_once(&errorPonce_, &HttpLoggingFilter::getErrorLogger);

        LogStream stream;
        const string& errMsg = response.errorMsg();
        const string& server = request.getHeader("Host");
        stream << "[" << FileLogger::formatTime(Timestamp::now()) << "] [error] "
               << response.statusCode() << " " << response.statusMessage() << ", "
               << "error msg: [" << (errMsg.empty() ? "-" : errMsg) << "], "
               << "client: " << conn->peerAddress().toIpPort() << ", "
               << "server: " << (server.empty() ? "-" : server) << ", "
               << "request: \"" << request.reqLineToString() << "\"\n";
        HTTPErrorLogger->info(stream);
    }

    return doNextFilter(config, conn, request, response);
}
