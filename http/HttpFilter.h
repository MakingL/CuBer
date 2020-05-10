#ifndef CUBER_HTTPFILTER_H
#define CUBER_HTTPFILTER_H

#include "base/noncopyable.h"
#include "net/TcpConnection.h"

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

    virtual HttpFilterState
    filter(const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) = 0;

    HttpFilterState
    doNextFilter(const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) {
        if (nextFilter_) {
            return nextFilter_->filter(conn, request, response);
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
    filter(const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) override {
        return doNextFilter(conn, request, response);
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
    filter(const TcpConnectionPtr &conn, const HttpRequest &req, HttpResponse &response) override;
};

class HttpWriteFilter : public AbstractHttpFilter {
public:
    explicit HttpWriteFilter() = default;

    explicit HttpWriteFilter(HttpFilterPtr nextFilter)
            : AbstractHttpFilter(nextFilter) {
    }

    HttpFilterState
    filter(const TcpConnectionPtr &conn, const HttpRequest &request, HttpResponse &response) override;
};

}
}
#endif //CUBER_HTTPFILTER_H