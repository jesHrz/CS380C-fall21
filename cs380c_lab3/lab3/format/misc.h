#ifndef CS380C_LAB3_MISC_H
#define CS380C_LAB3_MISC_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

template<typename T, typename... Args>
void err(T a, Args... args);
void err();
#define LOG(args...) do { std::cout << #args<< " -> "; err(args); } while (0)

#define ASSERT(eval) \
    do {             \
        if (!(eval)) \
            ERROR("Assertion failed:\n\t" #eval "\nat %s, %s, line %d.\n", __FILE__, __FUNCTION__, __LINE__);\
    } while(0)
#define ERROR(fmt, args...) \
    do {                             \
        fprintf(stderr, fmt, ##args);  \
        exit(EXIT_FAILED);           \
    } while(0)

#define StrList std::vector<std::string>
#define StrMap  std::map<int, std::string>
#define MK(args...) {args}
#define VSZ     8 // variable size

#define FIND_LIST(v, i)  (find((v).begin(), (v).end(), (i)))
#define CONTAIN_ITEM(v, i) (FIND_LIST(v, i) != (v).end())
#define FIND_VALUE(m, v) (std::find_if(m.begin(), m.end(), [v](const auto& i){return i.second==v;}))
#define CONTAIN_VALUE(m, v) (FIND_VALUE(m, v) != m.end())
#define CONTAIN_KEY(m, k) (m.count(k) != 0)

#define GET_REG(s) (str2int((s).substr(1, (s).length() - 1)))
#define LNR ((int)(instr - instrs_3addr.begin()) + 1)

#define FIND(s, p) ((s).find((p)) != std::string::npos)
#define SUBSTR(s, l, r) (s).substr((l), ((r) - (l)))

#define EXIT_SUCCESS 0
#define EXIT_FAILED  127

int str2int(const std::string s);
void split(const std::string &s, StrList &v, const char delim);
void trim(std::string &s);

#endif //CS380C_LAB3_MISC_H
