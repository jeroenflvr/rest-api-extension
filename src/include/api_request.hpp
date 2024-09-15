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


ConfigList load_config();
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
struct WebRequest {
    std::string method;
    std::string host;
    std::int32_t port;
    std::string path;
    std::string scheme;
    Headers headers;
 
    WebRequest(const std::string& h, const std::string& m = "GET", const std::string& s = "https", const std::string& path = "/",
            const std::int32_t port = 8080)
        : method(m), host(h), scheme(s), path(path), port(port) {}
 
    void addHeader(const std::string& key, const std::string& value) {
        headers.emplace_back(std::make_pair(key, value));
    }
 
    void printRequest() const {
        std:: cout << "Method: " << method << "\n"
                << "Host: " << host << "\n"
                << "Port: " << port << "\n"
                << "Path: " << path << "\n"
                << "Scheme: " << scheme << "\n"
                << "Headers: ";
            if (headers.empty()) {
                std::cout << "  No headers\n";
            } else {
                for (const auto& header: headers) {
                    std::cout << "  " << header.first << ": " << header.second << "\n";
                }
            }
    }

    std::string queryAPI();



};
#endif