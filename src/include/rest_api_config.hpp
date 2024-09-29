
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


namespace rest_api_config {

    std::vector<std::pair<std::string, std::string>> ParseOptionsFromJSON(const std::string &json_str);
}