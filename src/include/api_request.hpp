#ifndef API_REQUEST_HPP
#define API_REQUEST_HPP
#include <string>
// Function to query the API and return the response as a string
std::string query_api(const std::string& url, const std::string& token);
std::string get_env_string(const std::string& key);

 
using Header = std::pair<std::string, std::string>;
using Headers = std::vector<Header>;


// Define the Endpoint struct
struct Endpoint {
    std::string uri;
};

// Define the Endpoints struct containing multiple Endpoint objects
struct Endpoints {
    Endpoint data;
    Endpoint schema;
    // Add other endpoints as needed, e.g., info, details
};

// Define the Config struct containing host, port, and endpoints
struct Config {
    std::string host;
    std::string root_uri;
    int port;
    Endpoints endpoints;

};

// Define the ConfigItem struct containing name and config
struct ConfigItem {
    std::string name;
    Config config;
};

// Define the ConfigList as a vector of ConfigItem
using ConfigList = std::vector<ConfigItem>;

// Serialization for Endpoint
// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoint, uri)

// // Serialization for Endpoints
// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoints, data, schema)

// // Serialization for Config
// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, host, port, root_uri, endpoints)

// // Serialization for ConfigItem
// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigItem, name, config)

ConfigList load_config();

#endif