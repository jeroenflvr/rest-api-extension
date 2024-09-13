
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstdlib>  
 
using Header = std::pair<std::string, std::string>;
using Headers = std::vector<Header>;
 
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
};
 
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
 
std::string query_api(const std::string& url, const std::string& token) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
 
    std::string auth_header = "Authorization: Bearer ";
    auth_header += token;
 
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "accept: application/json");
        headers = curl_slist_append(headers, auth_header.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
 
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
 
    return readBuffer;
}