#pragma once
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

    struct SchemaEntry {
        std::string name;
        std::string type;
    };

    struct Config {
        std::string host;
        std::string root_uri;
        int port;
        Endpoints endpoints;
        std::vector<SchemaEntry> schema;
        int32_t page_size;
    };

    Config load_config(std::string filename, const std::string &api_name);
    std::vector<std::pair<std::string, std::string>> ParseOptionsFromJSON(const std::string &json_str);
}