#include "net/Buffer.h"
#include "http/HttpContext.h"

using namespace cuber;
using namespace cuber::net;

bool HttpContext::parseRequestLine(const char *begin, const char *end) {
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space)) {
        start = space + 1;
        space = std::find(start, end, ' ');
        if (space != end) {
            const char *question = std::find(start, space, '?');
            if (question != space) {
                request_.setPath(start, question);
                request_.setQuery(question, space);
            } else {
                request_.setPath(start, space);
            }
            start = space + 1;
            succeed = end - start == 8 && std::equal(start, end - 1, "HTTP/1.");
            if (succeed) {
                if (*(end - 1) == '1') {
                    request_.setVersion(HttpRequest::kHttp11);
                } else if (*(end - 1) == '0') {
                    request_.setVersion(HttpRequest::kHttp10);
                } else {
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}

HttpContext::HttpParseState HttpContext::parseHeaders(const char *begin, const char *end) {
    const char *colon = std::find(begin, end, ':');
    if (colon != end) {
        request_.addHeader(begin, colon, end);
    } else {
        if (begin == end) {
            // empty line, end of header
            return kHeaderEnd;
        }
        return kBadMessage;
    }

    return kParseAgain;
}

bool HttpContext::parseBody(Buffer *buf) {
    size_t content_length = 0;
    std::string content_length_str = request_.getHeader("Content-Length");
    if (!content_length_str.empty()) {
        content_length = static_cast<size_t>(stoi(content_length_str));
        if (content_length > buf->readableBytes()) {
            return false;
        }
        const char *begin = buf->peek();
        request_.setBody(begin, begin + content_length);
        buf->retrieve(content_length);
    } else {
        return false;
    }

    return true;
}

/*
 * 解析请求信息
 * 这里就是一个有限状态机
 * 解析收到的请求报文
 * */
HttpContext::HttpParseState HttpContext::parseRequest(Buffer *buf, Timestamp receiveTime) {

    bool hasMore = true;
    while (hasMore) {
        switch (state_) {
            case kExpectRequestLine: {
                const char *crlf = buf->findCRLF();
                if (crlf) {
                    if (parseRequestLine(buf->peek(), crlf)) {
                        request_.setReceiveTime(receiveTime);
                        buf->retrieveUntil(crlf + 2);
                        state_ = kExpectHeaders;
                    } else {
                        /* 报文格式错误 */
                        return kBadMessage;
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
                // if (std::string(request_.methodString()) == "POST") {
                //     // 延后读取 Body，将读取 body 的操作放到 readPostHandler 中完成
                //     if (!parseBody(buf)) {
                //         return kBadMessage;
                //     }
                // }
                state_ = kGotAll;
                break;
            }

            case kGotAll:
            default:
                return kParseOK;
        }
    }

    return kParseAgain;
}
