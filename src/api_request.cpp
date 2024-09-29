
#include <curl/curl.h>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstdlib>  
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <api_request.hpp>
#include <rest_api_config.hpp>

#include "logger.hpp"

using json = nlohmann::json;
 
using Header = std::pair<std::string, std::string>;
using Headers = std::vector<Header>;


 
std::string WebRequest::queryAPI(const std::string *body) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
 
    //std::cout << "executing WebRequest::queryAPI" << std::endl;
    logger.LOG_INFO("Executing WebRequest::queryAPI");
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
 


