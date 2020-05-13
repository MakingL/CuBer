#include <iostream>
#include <getopt.h>
#include "yaml-cpp/yaml.h"
#include "base/Logging.h"
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

class YamlLoader {
public:
    YamlLoader() = default;

    YAML::Node loadFile(const char *config_file) {
        YAML::Node node;
        try {
            node = YAML::LoadFile(config_file);
        } catch (const std::exception &e) {
            std::cerr << "Cannot parse configure file: '" << config_file << "'. Exception: " << e.what()
                      << std::endl;
            std::cerr << "Please check it for existence." << std::endl;
            exit(EXIT_FAILURE);
        }
        configFile_ = config_file;
        return node;
    }

    YAML::Node getNode(const YAML::Node &node, const std::string &node_name, bool exitOnUndefined = true) { /* 获取节点 */
        if (!node[node_name.c_str()].IsDefined()) {
            if (exitOnUndefined) {
                std::cerr << "Configure information: '" << node_name << "' isn't configured correctly" << std::endl;
                std::cerr << "Please configure it in file: " << configFile_ << std::endl;
                exit(EXIT_FAILURE);
            } else {
                return node;
            }
        }
        return node[node_name.c_str()];;
    }


    template<typename T>
    bool getVal(const YAML::Node &node, const std::string &property, T &val, bool exitOnUndefined = true) { /* 获得配置信息 */
        if (!node[property.c_str()].IsDefined()) {
            if (exitOnUndefined) {
                std::cerr << "Configure information: '" << property << "' isn't configured correctly" << std::endl;
                std::cerr << "Please configure it in file: " << configFile_ << std::endl;
                exit(EXIT_FAILURE);
            } else {
                return false;
            }
        }
        val = node[property.c_str()].as<T>();;
        return true;
    }

private:
    std::string configFile_;
};

ServerConfig::ServerConfig(const std::string &conf) : configPath_(conf) {

}

void ServerConfig::loadConfig() {
    YamlLoader yamlLoader = YamlLoader();
    YAML::Node configRootNode = yamlLoader.loadFile(configPath_.c_str());
    YAML::Node node;

    MainConfig main;
    node = yamlLoader.getNode(configRootNode, "main");
    yamlLoader.getVal(node, "max_workers", main.maxWorker);
    yamlLoader.getVal(node, "keepalive_timeout", main.keepAliveTimeout);
    yamlLoader.getVal(node, "worker_connections", main.maxWorkerConnections);
    yamlLoader.getVal(node, "access_log", main.accessLogPath, false);
    yamlLoader.getVal(node, "error_log", main.errorLogPath, false);
    LOG_DEBUG << "main config: ";
    LOG_DEBUG << "\tmax_workers: " << main.maxWorker;
    LOG_DEBUG << "\tkeepalive_timeout: " << main.keepAliveTimeout;
    LOG_DEBUG << "\tworker_connections: " << main.maxWorkerConnections;
    LOG_DEBUG << "\taccess_log: " << main.accessLogPath;
    LOG_DEBUG << "\terror_log: " << main.errorLogPath;
    setMainConf(main);

    node = yamlLoader.getNode(configRootNode, "upstream", false);
    LOG_DEBUG << "upstream config: ";
    if (!(node == configRootNode)) {
        for (auto upstreamNode : node) {
            Upstream upstreamInfo;
            yamlLoader.getVal(upstreamNode, "name", upstreamInfo.name);
            yamlLoader.getVal(upstreamNode, "server", upstreamInfo.address);
            yamlLoader.getVal(upstreamNode, "max_fails", upstreamInfo.maxFails);
            yamlLoader.getVal(upstreamNode, "fail_timeout", upstreamInfo.failTimeSecond);
            LOG_DEBUG << "\tname: " << upstreamInfo.name;
            LOG_DEBUG << "\tserver: " << upstreamInfo.address;
            LOG_DEBUG << "\tmax_fails: " << upstreamInfo.maxFails;
            LOG_DEBUG << "\tfail_timeout: " << upstreamInfo.failTimeSecond;
            addUpstream(upstreamInfo);
        }
    }

    Server server;
    node = yamlLoader.getNode(configRootNode, "server");
    LOG_DEBUG << "server config: ";
    for (auto server_node: node) {
        yamlLoader.getVal(server_node, "listen", server.listenPort);
        yamlLoader.getVal(server_node, "workers", server.workers);
        yamlLoader.getVal(server_node, "root_path", server.rootPath);
        yamlLoader.getVal(server_node, "index", server.indexFiles, false);
        LOG_DEBUG << "\tlisten: " << server.listenPort;
        LOG_DEBUG << "\tworkers: " << server.workers;
        LOG_DEBUG << "\troot_path: " << server.rootPath;
        LOG_DEBUG << "\tindex: ";
        for (const auto &indexFile : server.indexFiles) {
            LOG_DEBUG << "\t\t" << indexFile;
        }
        std::vector<std::string> proxyLocations;
        std::vector<std::string> staticLocations;

        YAML::Node proxy_nodes;
        if (!((proxy_nodes = yamlLoader.getNode(server_node, "proxy", false)) == server_node)) {
            LOG_DEBUG << "\tproxy: ";
            for (auto proxy_node : proxy_nodes) {
                ProxyServer proxyInfo;
                yamlLoader.getVal(proxy_node, "location", proxyInfo.location);
                yamlLoader.getVal(proxy_node, "proxy_pass", proxyInfo.upstreamName);
                yamlLoader.getVal(proxy_node, "proxy_set_header", proxyInfo.setHeaders, false);
                LOG_DEBUG << "\t\tlocation: " << proxyInfo.location;
                LOG_DEBUG << "\t\tproxy_pass: " << proxyInfo.upstreamName;
                addProxy(proxyInfo);

                if (!existUpstreamInfo(proxyInfo.upstreamName)) {
                    std::cerr << "Configure information is incomplete(Doesn't exists upstream information.) for proxy: "
                              << proxyInfo.upstreamName << std::endl;
                    exit(EXIT_FAILURE);
                }

                proxyLocations.push_back(proxyInfo.location);
            }
        }
        server.proxyLocations = proxyLocations;

        YAML::Node static_nodes;
        if (!((static_nodes = yamlLoader.getNode(server_node, "static", false)) == server_node)) {
            LOG_DEBUG << "\tstatic: ";
            for (auto static_node : static_nodes) {
                StaticInfo staticInfo;
                yamlLoader.getVal(static_node, "location", staticInfo.location);
                yamlLoader.getVal(static_node, "alias", staticInfo.alias);
                LOG_DEBUG << "\t\tlocation: " << staticInfo.location;
                LOG_DEBUG << "\t\talias: " << staticInfo.alias;
                addStatic(staticInfo);

                staticLocations.push_back(staticInfo.location);
            }
        }
        server.staticLocations = staticLocations;

        addServer(server);
    }
}

extern int optopt;
extern char *optarg;
static struct option long_options[] = {
        {"conf", required_argument, NULL, 'c'},
        {"help", no_argument,       NULL, 'h'}
};

void cuber::config::show_usage() {
    std::cout << "\nUsage: \n"
              << "\t CuBer -c yaml_config_file\n"
              << "\t CuBer -h | --help\n"
              << "\nOptions:\n"
              << "\t-c | --conf \t Set configure file\n"
              << "\t-h | --help \t show help\n";
}

bool cuber::config::parse_command_parameter(int argc, char *argv[], std::string &configPath) {
    bool success = false;
    int c = 0;
    while (EOF != (c = getopt_long(argc, argv, "hc:", long_options, NULL))) {
        switch (c) {
            case 'c':
                success = true;
                configPath = optarg;
                break;
            case 'h':
                std::cout << "\n\tCuBer, a light weight http server.\n";
                show_usage();
                exit(EXIT_FAILURE);
                /*success = false;
                break;*/
            case '?':
                success = false;
                std::cout << "\nUnknown option: " << optopt << "\n";
                break;
            default:
                success = false;
                std::cerr << "Unprocessed option: " << optopt << "\n";
                break;
        }
    }

    return success;
}
