
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <nlohmann/json.hpp>
#include "rest_api_config.hpp"

using json = nlohmann::json;

using ConfigList = std::vector<rest_api_config::ConfigItem>;



namespace rest_api_config {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoint, uri)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoints, data, schema)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, host, port, root_uri, endpoints)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigItem, name, config)


    std::vector<std::pair<std::string, std::string>> ParseOptionsFromJSON(const std::string &json_str) {
        std::vector<std::pair<std::string, std::string>> options;

        json parsed_json;
        try {
            parsed_json = json::parse(json_str);
        } catch (const json::parse_error &e) {
            throw std::invalid_argument(std::string("Failed to parse JSON options: ") + e.what());
        }

        if (!parsed_json.is_object()) {
            throw std::invalid_argument("JSON options must be an object with key-value pairs.");
        }

        for (auto it = parsed_json.begin(); it != parsed_json.end(); ++it) {
            if (!it.value().is_string()) {
                throw std::invalid_argument("All values in JSON options must be strings.");
            }
            options.emplace_back(it.key(), it.value().get<std::string>());
        }

        return options;
    }
    
    ConfigList load_config(std::string filename) {
        // TODO: move to config
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Unable to open config.json file.\n";
            return {};
        }

        json j;
        try {
            file >> j;
        } catch (json::parse_error& e) {
            std::cerr << "Parse error: " << e.what() << '\n';
            return {};
        }

        file.close();

        ConfigList configList;
        try {
            configList = j.get<ConfigList>();
        } catch (json::type_error& e) {
            std::cerr << "Type error during deserialization: " << e.what() << '\n';
            return {};
        } catch (json::exception& e) {
            std::cerr << "Error during deserialization: " << e.what() << '\n';
            return {};
        }

        for (const auto& item : configList) {
            std::cout << "Name: " << item.name << '\n';
            std::cout << "Host: " << item.config.host << '\n';
            std::cout << "Port: " << item.config.port << '\n';
            std::cout << "Root URI: " << item.config.root_uri << '\n';

            std::cout << "Endpoints:\n";
            std::cout << "  Data URI: " << item.config.endpoints.data.uri << '\n';
            std::cout << "  Schema URI: " << item.config.endpoints.schema.uri << '\n';
            std::cout << "--------------------------\n";
        }

        return configList;
    }



    
}