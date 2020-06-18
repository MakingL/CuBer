#ifndef CUBER_HTTPFILTER_H
#define CUBER_HTTPFILTER_H

#include "config/ServerConfig.h"
#include "base/noncopyable.h"
#include "net/TcpConnection.h"
#include "base/FileLogging.h"

using namespace cuber::config;

namespace cuber {
namespace net {
class HttpRequest;

class HttpResponse;

enum HttpFilterState {
    kFilterDone,
    kFilterError,
    kFilterAgain,
    kFilterDecline,
};

class AbstractHttpFilter : public cuber::noncopyable {
public:
    explicit AbstractHttpFilter() : nextFilter_(nullptr) {
    }

    explicit AbstractHttpFilter(AbstractHttpFilter *nextFilter) : nextFilter_(nextFilter) {
    }

    virtual ~AbstractHttpFilter() = default;

    void setNextFilter(AbstractHttpFilter *nextFilter)
    {
        nextFilter_.reset(nextFilter);
    }

    void appendFilter(AbstractHttpFilter *nextFilter)
    {
        auto filter = nextFilter_.get();
        while (filter->nextFilter_.get())
        {
            filter = filter->nextFilter_.get();
        }
        filter->setNextFilter(nextFilter);
    }

    virtual HttpFilterState
    filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) = 0;

    HttpFilterState
    doNextFilter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) {
        if (nextFilter_) {
            return nextFilter_->filter(config, conn, request, response);
        }
        return kFilterDone;
    }

private:
    std::unique_ptr<AbstractHttpFilter> nextFilter_;
};

typedef AbstractHttpFilter *HttpFilterPtr;

class HttpFilter : public AbstractHttpFilter {
public:
    explicit HttpFilter() : AbstractHttpFilter() {
    }

    explicit HttpFilter(HttpFilterPtr nextFilter)
            : AbstractHttpFilter(nextFilter) {
    }

    HttpFilterState
    filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) override {
        return doNextFilter(config, conn, request, response);
    }
};

class HttpHeadersFilter : public AbstractHttpFilter {
public:
    explicit HttpHeadersFilter() : AbstractHttpFilter() {
    }

    explicit HttpHeadersFilter(HttpFilterPtr nextFilter)
            : AbstractHttpFilter(nextFilter) {
    }

    HttpFilterState
    filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &req, HttpResponse &response) override;
};

class HttpWriteFilter : public AbstractHttpFilter {
public:
    explicit HttpWriteFilter() = default;

    explicit HttpWriteFilter(HttpFilterPtr nextFilter)
            : AbstractHttpFilter(nextFilter) {
    }

    HttpFilterState
    filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) override;
};

class HttpLoggingFilter : public AbstractHttpFilter {
public:
    explicit HttpLoggingFilter() = default;

    explicit HttpLoggingFilter(HttpFilterPtr nextFilter)
            : AbstractHttpFilter(nextFilter) {
    }

    HttpFilterState
    filter(const ServerConfig *config, const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) override;

private:
    static pthread_once_t accessPonce_;
    static pthread_once_t errorPonce_;

    static void getAccessLogger();
    static void getErrorLogger();

    static FileLoggerPtr HTTPAccessLogger;
    static FileLoggerPtr HTTPErrorLogger;
};

}
}
#endif //CUBER_HTTPFILTER_H
