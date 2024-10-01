
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <nlohmann/json.hpp>
#include "rest_api_config.hpp"

#include "logger.hpp"

using json = nlohmann::json;

// using ConfigList = std::vector<rest_api_config::ConfigItem>;



namespace rest_api_config {
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoint, uri)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoints, data, schema)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, host, port, root_uri, endpoints)

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SchemaEntry, name, type)


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
    
    Config load_config(std::string filename, const std::string &api_name) {
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

        // Iterate over the array of config items
        for (size_t i = 0; i < j.size(); ++i) {
            auto& item = j[i];

            // Check if the name matches the provided api_name
            if (item["name"].get<std::string>() == api_name) {
                logger.LOG_INFO("Name: " + item["name"].get<std::string>());
                logger.LOG_INFO("Host: " + item["config"]["host"].get<std::string>());
                logger.LOG_INFO("Port: " + std::to_string(item["config"]["port"].get<int>()));
                logger.LOG_INFO("Root URI: " + item["config"]["root_uri"].get<std::string>());

                logger.LOG_INFO("Endpoints:");
                logger.LOG_INFO("  Data URI: " + item["config"]["endpoints"]["data"]["uri"].get<std::string>());
                logger.LOG_INFO("  Schema URI: " + item["config"]["endpoints"]["schema"]["uri"].get<std::string>());

                // Check if the "schema" field exists in the current "config"
                Config config;
                try {
                    config = item["config"].get<Config>();
                } catch (json::type_error& e) {
                    std::cerr << "Type error during deserialization: " << e.what() << '\n';
                    return {};
                } catch (json::exception& e) {
                    std::cerr << "Error during deserialization: " << e.what() << '\n';
                    return {};
                }

                if (item["config"].contains("schema")) {
                    logger.LOG_INFO("Schema found:");
                    config.schema = item["config"]["schema"].get<std::vector<SchemaEntry>>();
                    for (const auto& entry : config.schema) {
                        logger.LOG_INFO("  Schema Entry - Name: " + entry.name + ", Type: " + entry.type);
                    }
                } else {
                    logger.LOG_INFO("No schema defined for " + api_name);
                }

                if (item["config"].contains("page_size")) {
                    config.page_size = item["config"]["page_size"].get<int>();
                    logger.LOG_INFO("Page size: " + std::to_string(config.page_size));
                } else {
                    config.page_size = 100; // Default page size if not specified
                }

                // Return the matching config
                return config;
            }
        }

        // If no match is found, return an empty Config object or handle it as needed
        std::cerr << "No configuration found for API: " << api_name << '\n';
        return {};
    }

    
}