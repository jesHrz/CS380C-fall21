#include "misc.h"

int str2int(const std::string s) {
    int ret;
    std::istringstream iss(s);
    iss >> ret;
    return ret;
}

void split(const std::string &s, StrList &v, const char delim) {
    v.clear();
    std::istringstream iss(s);
    for (std::string tmp; std::getline(iss, tmp, delim); ) {
        v.emplace_back(std::move(tmp));
    }
}

void trim(std::string &s) {
    if (s.empty())
        return;

    s.erase(0,s.find_first_not_of(' '));
    s.erase(s.find_last_not_of(' ') + 1);
}
