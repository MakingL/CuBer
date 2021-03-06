#ifndef CUBER_SERVERCONFIG_H
#define CUBER_SERVERCONFIG_H

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <string>
#include <utility>
#include <cassert>
#include <vector>

namespace cuber {
namespace config {

class LoggerConfig {
public:
    std::string loggerName;
    std::string logSavePath;
    long long rollFileSize;

    LoggerConfig() : loggerName("default"),
                    logSavePath("defaultLog"),
                    rollFileSize(0) {
    }

    LoggerConfig(std::string name, std::string path, long long rollSize)
                : loggerName(std::move(name)),
                logSavePath(std::move(path)), rollFileSize(rollSize) {
    }
};

class MainConfig
{
public:
    int maxWorker;
    int keepAliveTimeout;
    int maxWorkerConnections;
    std::unordered_map<std::string, LoggerConfig> loggers;

    explicit MainConfig(int workers = 4, int keepaliveTime = 65, int workerConn = 768)
                        : maxWorker(workers), keepAliveTimeout(keepaliveTime),
                        maxWorkerConnections(workerConn) {
    }

    explicit MainConfig(std::unordered_map<std::string, LoggerConfig> loggerMap,
                        int workers=4, int keepaliveTime = 65, int workerConn = 768)
                        : maxWorker(workers), keepAliveTimeout(keepaliveTime),
                        maxWorkerConnections(workerConn),
                        loggers(std::move(loggerMap))    {
    }

    void setLoggersInfo(const std::unordered_map<std::string, LoggerConfig>& loggersInfo) {
        loggers = loggersInfo;
    }
};

class ProxyServer
{
public:
    std::string location;
    std::string upstreamName;
    std::vector<std::string> setHeaders;

    ProxyServer() = default;

    ProxyServer(const std::string &loc, const std::string &name,
                const std::vector<std::string> &headers)
                : location(loc), upstreamName(name), setHeaders(headers)
    {
    }
};

class Upstream
{
public:
    std::string name;
    std::string address;
    int maxFails;
    int failTimeSecond;

    Upstream() : name(), address(), maxFails(3), failTimeSecond(30)
    {
    }

    Upstream(const std::string &Name, const std::string &Address, int max_fails, int fail_second);
};

class StaticInfo
{
public:
    std::string location;
    std::string alias;

    StaticInfo() = default;
    StaticInfo(const std::string &loc, const std::string &path);
};

typedef std::map<std::string, ProxyServer> ProxyMap;
typedef std::map<std::string, Upstream> UpstreamMap;
typedef std::map<std::string, StaticInfo> StaticsMap;

class Server
{
public:
    uint16_t listenPort;
    int workers;
    std::string rootPath;
    std::vector<std::string> indexFiles;
    std::vector<std::string> proxyLocations;
    std::vector<std::string> staticLocations;

    Server() = default;
    Server(uint16_t port, int worker_num, const std::string &root,
           const std::vector<std::string> &index,
           const std::vector<std::string> &proxyLocations,
           const std::vector<std::string> &staticLocations);
};

typedef std::map<uint16_t, Server> ServerMap;

class ServerConfig
{
public:
    ServerConfig() = default;
    explicit ServerConfig(const std::string &conf_path);

    void setMainConf(const MainConfig &conf)
    {
        serverMainConfig_ = conf;
    }

    const MainConfig &mainConf() const
    {
        return serverMainConfig_;
    }

    void addLoggerInfo(const std::string& name, const LoggerConfig& logger)
    {
        loggersConfig_[name] = logger;
    }

    std::unordered_map<std::string, LoggerConfig>& loggersInfo()
    {
        return loggersConfig_;
    }

    bool configuredLogger() const {
        return !loggersConfig_.empty();
    }

    bool existLogger(const std::string& name) const {
        return loggersConfig_.count(name);
    }

    void addServer(const Server &server)
    {
        servers_[server.listenPort] = server;
        listenedPorts_.insert(server.listenPort);
    }

    bool listenedPort(uint16_t port) const
    {
        return static_cast<bool>(listenedPorts_.count(port));
    };

    const Server &serverConf(uint16_t port)
    {
        assert(listenedPort(port));
        return servers_[port];
    }

    ServerMap serversConfMap() {
        return servers_;
    }

    void addProxy(const ProxyServer &proxy)
    {
        proxies_[proxy.location] = proxy;
        locations_.insert(proxy.location);
    }

    bool existProxy(const std::string &location)
    {
        return proxies_.count(location) > 0;
    }

    const ProxyServer &proxyInfo(const std::string &location)
    {
        assert(proxies_.count(location) > 0);
        return proxies_[location];
    }

    void addUpstream(const Upstream &upstream)
    {
        upstreams_[upstream.name] = upstream;
    }

    const Upstream &upstreamInfo(const std::string &name)
    {
        return upstreams_[name];
    }

    bool existUpstreamInfo(const std::string &name) {
        return upstreams_.count(name) > 0;
    }

    void addStatic(const StaticInfo &static_info)
    {
        statics_[static_info.location] = static_info;
        locations_.insert(static_info.location);
    }

    bool existStatic(const std::string &location)
    {
        return statics_.count(location) != 0;
    }

    const StaticInfo &getStaticInfo(const std::string &location)
    {
        assert(statics_.count(location));
        return statics_[location];
    }

    const std::string &staticPath(const std::string &location)
    {
        assert(statics_.count(location));
        return statics_[location].alias;
    }

    const std::unordered_set<std::string> &listenedLocations()
    {
        return locations_;
    }

    std::vector<std::string> indexFiles(uint16_t listenPort)
    {
        if (!servers_.count(listenPort))
        {
            return {};
        }
        auto server = servers_[listenPort];
        return server.indexFiles;
    }

    void loadConfig();

private:
    std::string configPath_;
    std::unordered_set<int> listenedPorts_;
    std::unordered_set<std::string> locations_;
    MainConfig serverMainConfig_;
    std::unordered_map<std::string, LoggerConfig> loggersConfig_;
    ServerMap servers_;
    StaticsMap statics_;
    ProxyMap proxies_;
    UpstreamMap upstreams_;
};

void show_usage();
bool parse_command_parameter(int argc, char *argv[], std::string &configPath);

} // namespace config
} // namespace cuber

#endif //CUBER_SERVERCONFIG_H
