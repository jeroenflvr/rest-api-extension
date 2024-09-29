

#include <string>
#include <vector> 

namespace helpers {
    bool contains(const std::vector<std::string>& vec, const std::string& target);
    std::string toLower(const std::string& str);
    void removeBeforeSelect(std::string& query);
    bool startsWithCaseInsensitive(const std::string& str, const std::string& prefix);

}