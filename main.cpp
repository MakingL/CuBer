#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "net/EventLoop.h"
#include "base/TimeZone.h"
#include "base/Logging.h"
#include "config/ServerConfig.h"

#include <iostream>
#include <map>

using namespace cuber;
using namespace cuber::net;
using namespace cuber::config;

int main(int argc, char *argv[]) {
    cuber::TimeZone beijing(8 * 3600, "CST");
    cuber::Logger::setTimeZone(beijing);

    int numThreads = 0;
    if (argc > 1) {
        Logger::setLogLevel(Logger::WARN);
        numThreads = atoi(argv[1]);
    }

    ServerConfig serverConf("conf/cuber.yaml");
    serverConf.loadConfig();
    auto server_info = serverConf.serverConf(8080);

    EventLoop loop;
    HttpServer server(&loop, InetAddress(server_info.listenPort), "CuBer", &serverConf);
    server.setThreadNum(numThreads);
    server.start();
    loop.loop();
}