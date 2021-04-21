#ifndef ARKCORE_UTIL_HPP
#define ARKCORE_UTIL_HPP

#include <sstream>
#include <string>
#include <iostream>
#include <vector>

enum LogLevel{
    debug,info,warning,error,fatal,
};

LogLevel log_level=info;

void set_log_mode(LogLevel level) {
    log_level = level;
}

inline std::ostream & lprintf(std::ostream & ostr, const char * fstr) noexcept {
    return ostr << fstr;
}

template<typename T, typename... Args>
std::ostream & lprintf(std::ostream & ostr,
                       const char * fstr, const T & x) noexcept {
    size_t i = 0;
    char c = fstr[0];

    while (c != '%') {
        if (c == 0) return ostr; // string is finished
        ostr << c;
        c = fstr[++i];
    };
    c = fstr[++i];
    ostr << x;

    if (c == 0) return ostr; //

    // print the rest of the stirng
    ostr << &fstr[++i];
    return ostr;
}

template<typename T, typename... Args>
std::ostream & lprintf(std::ostream & ostr,
                       const char * fstr, const T & x, Args... args) noexcept {
    size_t i = 0;
    char c = fstr[0];

    while (c != '%') {
        if (c == 0) return ostr;
        ostr << c;
        c = fstr[++i];
    };
    c = fstr[++i];
    ostr << x;

    if (c == 0) return ostr;

    return lprintf(ostr, &fstr[++i], args...);
}

template<typename... args>
void linfof(const char *pattern) {
    if (log_level <= info)
        lprintf(std::cout, pattern);
}
template<typename... args>
void ldebugf(const char *pattern) {
    if (log_level <= debug)
        lprintf(std::cout, pattern);
}
template<typename... args>
void lwarningf(const char *pattern) {
    if (log_level <= warning)
        lprintf(std::cout,pattern);
}
template<typename... args>
void lerrorf(const char *pattern) {
    if (log_level <= error)
        lprintf(std::cout,pattern);
}
template<typename... args>
void lfatalf(const char *pattern) {
    if (log_level <= fatal)
        lprintf(std::cout,pattern);
}
template<typename... args>
void linfof(const char *pattern,args... a) {
    if (log_level <= info)
        lprintf(std::cout,pattern, a...);
}
template<typename... args>
void ldebugf(const char *pattern,args... a) {
    if (log_level <= debug)
        lprintf(std::cout,pattern, a...);
}
template<typename... args>
void lwarningf(const char *pattern,args... a) {
    if (log_level <= warning)
        lprintf(std::cout,pattern, a...);
}
template<typename... args>
void lerrorf(const char *pattern,args... a) {
    if (log_level <= error)
        lprintf(std::cout,pattern, a...);
}
template<typename... args>
void lfatalf(const char *pattern,args... a) {
    if (log_level <= fatal)
        lprintf(std::cout,pattern, a...);
}


void split(const std::string& s,
           std::vector<std::string>& sv,
           const char delim = ' ') {
    sv.clear();
    std::istringstream iss(s);
    std::string temp;

    while (std::getline(iss, temp, delim)) {
        sv.emplace_back(std::move(temp));
    }

}


std::vector<std::string> Split(std::string s,const char delim) {
    std::vector<std::string> v;
    split(s, v, delim);
    return v;
}


#endif //ARKCORE_UTIL_HPP
