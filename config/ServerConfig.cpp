#include "config/ServerConfig.h"

using namespace cuber;
using namespace cuber::config;

StaticInfo::StaticInfo(const std::string &loc, const std::string &path)
    : location(loc), alias(path) {
}

Upstream::Upstream(const std::string &Name, const std::string &Address, int max_fails, int fail_second)
        : name(Name), address(Address), maxFails(max_fails),
          failTimeSecond(fail_second) {
}

Server::Server(uint16_t port, int worker_num, const std::string &root,
               const std::vector<std::string> &index,
               const std::vector<std::string> &proxies,
               const std::vector<std::string> &statics) : listenPort(port),
                                                          workers(worker_num), rootPath(root),
                                                          indexFiles(index), proxyLocations(proxies),
                                                          staticLocations(statics) {
}

ServerConfig::ServerConfig(const std::string &conf) : configPath_(conf) {

}

void ServerConfig::loadConfig() {
    MainConfig main(8, 65, 768,
                    "/home/mlee/CuBer/www/logs/access.log",
                    "/home/mlee/CuBer/www/logs/error.log");
    setMainConf(main);

    std::vector<std::string> index{"index.html",};
    std::vector<std::string> proxy{"/echo/", "/django/",};
    std::vector<std::string> statics{"/html/", "/static/"};
    std::string root("/home/mlee/CuBer/");
    Server server(8080, 2, root, index,
                  proxy, statics);
    addServer(server);

    StaticInfo static1("/html/", "/home/mlee/CuBer/www/html/");
    StaticInfo static2("/static/", "/home/mlee/CuBer/www/static/");
    addStatic(static1);
    addStatic(static2);

    std::vector<std::string> setHeader{"X-Real-IP", "X-Forwarded-For"};
    ProxyServer proxy1("/echo/", "echo_backend", setHeader);
    ProxyServer proxy2("/counter/", "count_backend", setHeader);
    addProxy(proxy1);
    addProxy(proxy2);

    Upstream upstream1("echo_backend", "127.0.0.1:8081", 3, 30);
    Upstream upstream2("count_backend", "127.0.0.1:8081", 3, 30);
    addUpstream(upstream1);
    addUpstream(upstream2);
}
