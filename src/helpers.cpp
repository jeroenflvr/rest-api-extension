#include <string>
#include <vector> 
#include "helpers.hpp"

namespace helpers {

    bool contains(const std::vector<std::string>& vec, const std::string& target) {
        return std::find(vec.begin(), vec.end(), target) != vec.end();
    }

    std::string toLower(const std::string& str) {
        std::string lower_str = str;
        std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
        return lower_str;
    }

    // Function to remove everything up to and including "SELECT" (case-insensitive)
    void removeBeforeSelect(std::string& query) {
        std::string keyword = "SELECT";
        
        // Convert query and keyword to lowercase for case insensitive comparison
        std::string lower_query = toLower(query);
        std::string lower_keyword = toLower(keyword);

        // Find the position of "SELECT" in the query
        size_t pos = lower_query.find(lower_keyword);
        if (pos != std::string::npos) {
            // Erase everything before and including "SELECT"
            query.erase(0, pos);
        }
    }

    bool startsWithCaseInsensitive(const std::string& str, const std::string& prefix) {
        if (str.size() < prefix.size()) {
            return false;
        }

        // Create lowercase copies of the string and prefix
        std::string str_lower = str.substr(0, prefix.size());
        std::string prefix_lower = prefix;

        std::transform(str_lower.begin(), str_lower.end(), str_lower.begin(), ::tolower);
        std::transform(prefix_lower.begin(), prefix_lower.end(), prefix_lower.begin(), ::tolower);

        return str_lower == prefix_lower;
    }
}