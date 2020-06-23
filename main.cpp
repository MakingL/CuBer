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
    std::string configPath;
    if (!cuber::config::parse_command_parameter(argc, argv, configPath)) {
        std::cout << "Cuber Need a configure file.\n";
        cuber::config::show_usage();
        exit(EXIT_FAILURE);
    }

#ifndef PRESSURE_TEST
    cuber::TimeZone beijing(8 * 3600, "CST");
    cuber::Logger::setTimeZone(beijing);
    Logger::setLogLevel(Logger::WARN);
#else
    Logger::setLogLevel(Logger::FATAL);
#endif

    ServerConfig serverConf(configPath);
    serverConf.loadConfig();
    ServerMap serversConfMap = serverConf.serversConfMap();
    if (serversConfMap.empty()) {
        std::cout << "No server is configured. Please configure server domain.\n";
        exit(EXIT_FAILURE);
    }
    auto server_info = serversConfMap.begin()->second;

    EventLoop loop;
    HttpServer server(&loop, InetAddress(server_info.listenPort), "CuBer", &serverConf);
    server.setThreadNum(server_info.workers);
    server.start();
    loop.loop();
}