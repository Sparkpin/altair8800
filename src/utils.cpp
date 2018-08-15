#include <sstream>
#include "utils.h"

std::vector<std::string> Altair8008Utils::splitString(const std::string& toSplit, char delim) {
    std::vector<std::string> res;
    std::istringstream splitStream(toSplit);
    std::string buf;
    while (std::getline(splitStream, buf, delim)) {
        res.push_back(buf);
    }
    return res;
}
