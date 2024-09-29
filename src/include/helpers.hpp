

#include <string>

namespace helpers {

    std::string toLower(const std::string& str);
    void removeBeforeSelect(std::string& query);
    bool startsWithCaseInsensitive(const std::string& str, const std::string& prefix);

}