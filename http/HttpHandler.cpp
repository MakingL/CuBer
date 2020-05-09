#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <functional>
#include "base/Logging.h"
#include "base/FileUtil.h"
#include "config/ServerConfig.h"
#include "net/TcpConnection.h"
#include "http/HttpHandler.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpProxy.h"

using namespace cuber;
using namespace cuber::net;

HttpHandleState
HttpAccessHandler::handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request,
                          HttpResponse &response) {
    const std::string &path = request.path();
    if (path.empty()) {
        return kHandleAgain;
    }

    /* 接着上次匹配的结果，找更短的 location， 对同一请求， 这个函数可能会被多次执行 */
    std::string reqPath(path.substr(0, request.locationEndPos()));
    if (reqPath.empty()) {
        response.setStatusCode(HttpResponse::k404NotFound);
        return kHandleDone;
    }

    const std::unordered_set<std::string> &serverLocationSet = config->listenedLocations();
    do {
        if (serverLocationSet.count(reqPath)) {
            request.setLocationEndPos(reqPath.size() - 1);
            break;
        }

        if (reqPath.size() == 1) {
            response.setStatusCode(HttpResponse::k404NotFound);
            return kHandleDone;
        }

        unsigned long new_pos = reqPath.size() >= 2 ? (reqPath.size() - 2) : 0;
        auto pos = reqPath.rfind('/', new_pos);
        if (pos == std::string::npos) {
            response.setStatusCode(HttpResponse::k404NotFound);
            return kHandleDone;
        }
        reqPath = reqPath.substr(0, pos + 1);
    } while (!reqPath.empty());

    return doHandleNext(config, conn, request, response);
}

HttpHandleState
HttpPostReadHandler::handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request,
                            HttpResponse &response) {

    if (request.method() == HttpRequest::kPost) {
        size_t content_length = 0;
        std::string content_length_str = request.getHeader("Content-Length");
        if (!content_length_str.empty()) {
            Buffer *buf = conn->inputBuffer();
            try {
                content_length = static_cast<size_t>(stoi(content_length_str));
            }
            catch (...) {
                response.setStatusCode(HttpResponse::k400BadRequest);
                return kHandleError;
            }
            if (content_length > buf->readableBytes()) {
                return kHandleAgain;
            }
            /* FIXME: 考虑 body 过大时，将 body 写入临时文件 */
            const char *begin = buf->peek();
            request.setBody(begin, begin + content_length);
            buf->retrieve(content_length);
        }
    }
    return doHandleNext(config, conn, request, response);
}

HttpHandleState
HttpStaticHandler::handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request,
                          HttpResponse &response) {
    if (request.method() != HttpRequest::kGet && request.method() != HttpRequest::kHead) {
        return doHandleNext(config, conn, request, response);
    }

    const std::string &path = request.path();
    std::string location(path.substr(0, request.locationEndPos() + 1));
    if (!config->existStatic(location)) {
        return doHandleNext(config, conn, request, response);
    }

    std::string staticPath(config->staticPath(location));
    std::string fileDir(staticPath.append(path.substr(request.locationEndPos() + 1)));

    const InetAddress &localAddress = conn->localAddress();
    uint16_t listenPort = localAddress.toPort();
    FileUtil::FileStat fileStat(fileDir.c_str());
    std::string filePath(fileDir);
    if (!fileStat.isfile()) {
        /* 假设 path 为 / 结尾 */
        if (filePath.empty() || filePath[filePath.size() - 1] != '/')
            return kHandleAgain;
        bool find_target = false;
        for (const auto &index : config->indexFiles(listenPort)) {
            filePath = fileDir.append(index);
            if (fileStat.isfile(filePath.c_str())) {
                find_target = true;
                break;
            }
        }
        if (!find_target)
            return kHandleAgain;
    }

    response.setStatusCode(HttpResponse::k200Ok);
    string mime_type;
    auto dot_pos = filePath.find_last_of('.');
    if (dot_pos == std::string::npos) {
        mime_type = MimeType::getMime("default");
    } else {
        mime_type = MimeType::getMime(filePath.substr(dot_pos));
    }

    response.setContentType(mime_type);

    if (request.method() == HttpRequest::kHead) {
        return kHandleDone;
    }

    /* 打开文件，并将文件映射到内存中 */
    int src_fd = open(filePath.c_str(), O_RDONLY, 0);
    if (src_fd < 0) {
        /* 文件不可读 */
        response.setStatusCode(HttpResponse::k404NotFound);
        return kHandleDecline;
    }

    auto file_size = static_cast<size_t>(fileStat.size());
    void *mmapRet = mmap(nullptr, file_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
    close(src_fd);
    if (mmapRet == MAP_FAILED) {
        munmap(mmapRet, file_size);
        response.setStatusCode(HttpResponse::k500InternalServerError);
        return kHandleError;
    }

    char *src_addr = static_cast<char *>(mmapRet);
    response.setBody(string(src_addr, src_addr + file_size));
    munmap(mmapRet, file_size);

    return kHandleDone;
}

HttpHandleState HttpDefaultHandler::handle(ServerConfig *config, const TcpConnectionPtr &conn, HttpRequest &request,
                                           HttpResponse &response) {
    response.setStatusCode(HttpResponse::k404NotFound);
    return kHandleDone;
}
