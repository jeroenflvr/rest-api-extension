#ifndef API_REQUEST_HPP
#define API_REQUEST_HPP
#include <string>
// Function to query the API and return the response as a string
std::string query_api(const std::string& url, const std::string& token);
std::string get_env_string(const std::string& key);
#endif