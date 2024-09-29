
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


namespace rest_api_config {

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
}