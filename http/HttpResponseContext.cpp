#include "net/Buffer.h"
#include "http/HttpResponseContext.h"

using namespace cuber;
using namespace cuber::net;

HttpResponseContext::HttpParseState HttpResponseContext::parseResponse(Buffer *buf, Timestamp receiveTime) {
    bool hasMore = true;
    while (hasMore) {
        switch (state_) {
            case kExpectStatusLine: {
                const char *crlf = buf->findCRLF();
                if (crlf) {
                    switch (parseStatusLine(buf->peek(), crlf)) {
                        case kParseOK:
                            hasMore = true;
                            buf->retrieveUntil(crlf + 2);
                            state_ = kExpectHeaders;
                            break;
                        case kBadMessage:
                            buf->retrieveUntil(crlf + 2);
                            return kBadMessage;
                        case kParseAgain:
                        default:
                            hasMore = false;
                            break;
                    }
                } else {
                    hasMore = false;
                }
                break;
            }
            case kExpectHeaders: {
                const char *crlf = buf->findCRLF();
                if (crlf) {
                    switch (parseHeaders(buf->peek(), crlf)) {
                        case kHeaderEnd:
                            state_ = kExpectBody;
                            break;
                        case kBadMessage:
                            buf->retrieveUntil(crlf + 2);
                            return kBadMessage;
                        case kParseAgain:
                        default:
                            break;
                    }
                    buf->retrieveUntil(crlf + 2);
                } else {
                    hasMore = false;
                }
                break;
            }
            case kExpectBody: {
                switch (parseBody(buf)) {
                    case kParseOK:
                        state_ = kGotAll;
                        break;
                    case kParseAgain:
                        break;
                    default:
                        return kBadMessage;
                }
                break;
            }
            case kGotAll:
            default:
                return kParseOK;
        }
    }
    return kParseAgain;
}

HttpResponseContext::HttpParseState HttpResponseContext::parseStatusLine(const char *begin, const char *end) {
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end) {
        response_.setVersion(start, space);
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            std::string status(start, space);
            int code = 0;
            try {
                code = std::stoi(status);
            }
            catch (...) {
                return kBadMessage;
            }

            response_.setStatusCode(static_cast<HttpResponse::HttpStatusCode>(code));

            start = space + 1;
            if (start != end) {
                response_.setStatusMessage(start, end);
                return kParseOK;
            }
        }
    }
    return kBadMessage;
}

HttpResponseContext::HttpParseState HttpResponseContext::parseHeaders(const char *begin, const char *end) {
    const char *colon = std::find(begin, end, ':');
    if (colon != end) {
        response_.addHeader(begin, colon, end);
    } else {
        if (begin == end) {
            return kHeaderEnd;
        }
        return kBadMessage;
    }

    return kParseAgain;
}

HttpResponseContext::HttpParseState HttpResponseContext::parseBody(Buffer *buf) {
    size_t content_length = 0;
    std::string content_length_str = response_.getHeader("Content-Length");
    if (!content_length_str.empty()) {
        try {
            content_length = static_cast<size_t >(stoi(content_length_str));
        } catch (...) {
            return kBadMessage;
        }
        if (content_length > buf->readableBytes()) {
            return kParseAgain;
        }
        const char *begin = buf->peek();
        response_.setBody(begin, begin + content_length);
        buf->retrieve(content_length);
    }

    return kParseOK;
}
