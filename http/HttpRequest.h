#ifndef CUBER_NET_HTTP_HTTPREQUEST_H
#define CUBER_NET_HTTP_HTTPREQUEST_H

#include "base/copyable.h"
#include "base/Timestamp.h"
#include "base/Types.h"

#include <map>
#include <cassert>
#include <cstdio>


namespace cuber {
    namespace net {
        /*
         * HTTP Request message
         * */
        class HttpRequest : public cuber::copyable {
        public:
            enum Method {
                kInvalid = 0, kGet, kPost, kHead, kPut, kDelete, kTrace, kOptions
            };

            enum Version {
                kUnknown = 0, kHttp10, kHttp11
            };

            HttpRequest()
                    : method_(kInvalid),
                      version_(kUnknown),
                      locationEndPos_(0) {
            }

            void setVersion(Version v) {
                version_ = v;
            }

            const char *versionString() const {
                const char *result = "UNKNOWN";
                switch (version_) {
                    case kHttp10:
                        result = "HTTP/1.0";
                        break;
                    case kHttp11:
                        result = "HTTP/1.1";
                        break;
                    default:
                        break;
                }
                return result;
            }

            Version getVersion() const { return version_; }

            bool setMethod(const char *start, const char *end) {
                assert(method_ == kInvalid);
                string m(start, end);
                if (m == "GET") {
                    method_ = kGet;
                } else if (m == "POST") {
                    method_ = kPost;
                } else if (m == "HEAD") {
                    method_ = kHead;
                } else if (m == "PUT") {
                    method_ = kPut;
                } else if (m == "DELETE") {
                    method_ = kDelete;
                } else if (m == "TRACE") {
                    method_ = kTrace;
                } else if (m == "OPTIONS") {
                    method_ = kOptions;
                } else {
                    method_ = kInvalid;
                }
                return method_ != kInvalid;
            }

            Method method() const { return method_; }

            const char *methodString() const {
                const char *result = "UNKNOWN";
                switch (method_) {
                    case kGet:
                        result = "GET";
                        break;
                    case kPost:
                        result = "POST";
                        break;
                    case kHead:
                        result = "HEAD";
                        break;
                    case kPut:
                        result = "PUT";
                        break;
                    case kDelete:
                        result = "DELETE";
                        break;
                    case kTrace:
                        result = "TRACE";
                        break;
                    case kOptions:
                        result = "OPTIONS";
                        break;
                    default:
                        break;
                }
                return result;
            }

            void setPath(const char *start, const char *end) {
                path_.assign(start, end);
                locationEndPos_ = path_.size();
            }

            void setPath(const string &p) {
                path_.assign(p);
                locationEndPos_ = path_.size();
            }

            const string &path() const { return path_; }

            void setLocationEndPos(std::string::size_type pos) {
                locationEndPos_ = pos;
            }

            std::string::size_type locationEndPos() {
                return locationEndPos_;
            }

            void setQuery(const char *start, const char *end) {
                query_.assign(start, end);
            }

            const string &query() const { return query_; }

            void setBody(const char *start, const char *end) {
                body_.assign(start, end);
            }

            const string &body() const {
                return body_;
            }

            void setReceiveTime(Timestamp t) { receiveTime_ = t; }

            Timestamp receiveTime() const { return receiveTime_; }

            void addHeader(const char *start, const char *colon, const char *end) {
                string field(start, colon);
                ++colon;
                while (colon < end && isspace(*colon)) {
                    ++colon;
                }
                string value(colon, end);
                while (!value.empty() && isspace(value[value.size() - 1])) {
                    value.resize(value.size() - 1);
                }
                headers_[field] = value;
            }

            void addHeader(const std::string &field, const std::string &val) {
                headers_[field] = val;
            }

            string getHeader(const string &field) const {
                string result;
                auto it = headers_.find(field);
                if (it != headers_.end()) {
                    result = it->second;
                }
                return result;
            }

            const std::map<string, string> &headers() const { return headers_; }

            void swap(HttpRequest &that) {
                std::swap(method_, that.method_);
                std::swap(version_, that.version_);
                path_.swap(that.path_);
                query_.swap(that.query_);
                receiveTime_.swap(that.receiveTime_);
                headers_.swap(that.headers_);
                std::swap(body_, that.body_);
            }

            void appendToBuffer(Buffer *output) const {
                /* request line */
                char buf[64] = {0};
                snprintf(buf, sizeof(buf), "%s ", methodString());
                output->append(buf);
                output->append(path_);
                bzero(buf, sizeof(buf));
                snprintf(buf, sizeof(buf), " %s\r\n", versionString());
                output->append(buf);

                /* Headers */
                for (const auto &header : headers_) {
                    output->append(header.first);
                    output->append(": ");
                    output->append(header.second);
                    output->append("\r\n");
                }

                output->append("\r\n");
                /* Request body */
                if (!body_.empty()) {
                    output->append(body_);
                    output->append("\r\n");
                }
            }

        private:
        private:
            Method method_;
            Version version_;
            string path_;
            std::string::size_type locationEndPos_;
            string query_;
            Timestamp receiveTime_;
            std::map<string, string> headers_;
            string body_;
        };

    }  // namespace net
}  // namespace cuber

#endif  // CUBER_NET_HTTP_HTTPREQUEST_H
