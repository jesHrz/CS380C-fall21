#ifndef CS380C_LAB1_MISC_H
#define CS380C_LAB1_MISC_H

#include <sstream>
#include <string>
#include <vector>

#define StrList std::vector<std::string>
#define StrMap  std::map<int, std::string>
#define MK(args...) {args}
#define VSZ     8 // variable size

#define EXIT_SUCCESS 0
#define EXIT_FAILED  127

int str2int(const std::string s);
void split(const std::string &s, StrList &v, const char delim);
void trim(std::string &s);

#endif //CS380C_LAB1_MISC_H
