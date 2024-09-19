
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstdlib>  
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <api_request.hpp>


using json = nlohmann::json;
 
using Header = std::pair<std::string, std::string>;
using Headers = std::vector<Header>;


using ConfigList = std::vector<ConfigItem>;

// nlohmann serialization macros
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoint, uri)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Endpoints, data, schema)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, host, port, root_uri, endpoints)

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ConfigItem, name, config)


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


 
std::string WebRequest::queryAPI(const std::string *body) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
 
    std::cout << "executing WebRequest::queryAPI" << std::endl;
    // std::string auth_header = "Authorization: Bearer ";
    // auth_header += token;


 
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, host.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "accept: application/json");
        // headers = curl_slist_append(headers, auth_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        if (body != nullptr){

            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body->c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body->size());
        }


        res = curl_easy_perform(curl);
 
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
 
    return readBuffer;
}



std::string get_env_string(const std::string& key) {
    const char* env_value = std::getenv(key.c_str());
    if (!env_value) {
        std::cerr << "Error: " << std::uppercase << key << "environment variable is not set." << std::endl;
        return "NA";
    }
 
    return env_value;
}
 
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
 
// std::string query_api(const std::string& url, const std::string& token) {
//     CURL* curl;
//     CURLcode res;
//     std::string readBuffer;
 
//     std::string auth_header = "Authorization: Bearer ";
//     auth_header += token;
 
//     curl = curl_easy_init();
//     if (curl) {
//         curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//         curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
//         curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
//         struct curl_slist *headers = NULL;
//         headers = curl_slist_append(headers, "accept: application/json");
//         headers = curl_slist_append(headers, auth_header.c_str());
//         curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
 
//         curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//         curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
//         res = curl_easy_perform(curl);
 
//         if (res != CURLE_OK) {
//             std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
//         }
//         curl_slist_free_all(headers);
//         curl_easy_cleanup(curl);
//     }
 
//     return readBuffer;
// }

