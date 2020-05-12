#include "http/HttpResponse.h"
#include "net/Buffer.h"

#include <cstdio>

using namespace cuber;
using namespace cuber::net;

pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime_;

void MimeType::init() {
    mime_[".html"] = "text/html";
    mime_[".avi"] = "video/x-msvideo";
    mime_[".bmp"] = "image/bmp";
    mime_[".c"] = "text/plain";
    mime_[".doc"] = "application/msword";
    mime_[".gif"] = "image/gif";
    mime_[".gz"] = "application/x-gzip";
    mime_[".htm"] = "text/html";
    mime_[".ico"] = "image/x-icon";
    mime_[".jpg"] = "image/jpeg";
    mime_[".png"] = "image/png";
    mime_[".txt"] = "text/plain";
    mime_[".mp3"] = "audio/mp3";
    mime_[".json"] = "application/json";
    mime_[".js"] = "text/javascript";
    mime_[".css"] = "text/css";
    mime_["default"] = "text/html";
}

std::string MimeType::getMime(const std::string &suffix) {
    pthread_once(&once_control, MimeType::init);
    if (mime_.find(suffix) == mime_.end())
        return mime_["default"];
    else
        return mime_[suffix];
}

HttpResponse::HttpResponse(bool close) : statusCode_(kUnknown),
                                         version_("HTTP/1.1"),
                                         closeConnection_(close) {
    statusMessageMap_[kUnknown] = "Unknown";
    statusMessageMap_[k200Ok] = "OK";
    statusMessageMap_[k202NoContent] = "No Content";
    statusMessageMap_[k206PartialContent] = "Partial Content";
    statusMessageMap_[k301MovedPermanently] = "Moved Permanently";
    statusMessageMap_[k302Found] = "Found";
    statusMessageMap_[k303SeeOther] = "SeeOther";
    statusMessageMap_[k304NotModified] = "Not Modified";
    statusMessageMap_[k307TemporaryRedirect] = "Temporary Redirect";
    statusMessageMap_[k400BadRequest] = "Bad Request";
    statusMessageMap_[k401Unauthorized] = "Unauthorized";
    statusMessageMap_[k403Forbidden] = "Forbidden";
    statusMessageMap_[k404NotFound] = "Not Found";
    statusMessageMap_[k500InternalServerError] = "Internal Server Error";
    statusMessageMap_[k502BadGateway] = "Bad Gateway";
    statusMessageMap_[k503ServiceUnavailable] = "Service Unavailable";

    bodyMessageMap_[k401Unauthorized] = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                                        "<title>Unauthorized</title></head><body>"
                                        "<h1>Unauthorized</h1></body></html>";
    bodyMessageMap_[k403Forbidden] = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                                     "<title>Forbidden</title></head><body>"
                                     "<h1>Forbidden</h1></body></html>";
    bodyMessageMap_[k404NotFound] = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                                    "<title>404 Not Found</title></head><body>"
                                    "<h1>Not Found</h1></body></html>";
    bodyMessageMap_[k500InternalServerError] = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                                               "<title>Internal Server Error</title></head><body>"
                                               "<h1>500 Internal Server Error</h1></body></html>";
    bodyMessageMap_[k502BadGateway] = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                                      "<title>Bad Gateway</title></head><body>"
                                      "<h1>502 Bad Gateway</h1></body></html>";
    bodyMessageMap_[k503ServiceUnavailable] = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">"
                                              "<title>Service Unavailable</title></head><body>"
                                              "<h1>503 Service Unavailable</h1></body></html>";
}

void HttpResponse::appendToBuffer(Buffer *output) const {
    /* Status Line */
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "%s %d ", version_.c_str(), statusCode_);
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    /* Response Headers */
    if (!body_.empty() && !headers_.count("Content-Length")) {
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
    }

    for (const auto &header : headers_) {
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");
    /* Response Body */
    if (!body_.empty()) {
        output->append(body_);
    }
}

void HttpResponse::addHeader(const char *start, const char *colon, const char *end) {
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

string HttpResponse::getHeader(const string &field) {
    string result;
    if (headers_.count(field)) {
        result = headers_[field];
    }
    return result;
}
