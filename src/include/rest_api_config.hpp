
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


namespace rest_api_config {
    struct Endpoint {
        std::string uri;
    };

    // TODO: static endpoints definitions, might not be a good idea??
    struct Endpoints {
        Endpoint data;
        Endpoint schema;
    };


    struct Config {
        std::string host;
        std::string root_uri;
        int port;
        Endpoints endpoints;
    };

    struct ConfigItem {
        std::string name;
        Config config;
    };

    using ConfigList = std::vector<ConfigItem>;


    ConfigList load_config(std::string filename, std::string &api_name);
    std::vector<std::pair<std::string, std::string>> ParseOptionsFromJSON(const std::string &json_str);
    ConfigItem* findConfigByName(ConfigList& configList, const std::string& name);
}