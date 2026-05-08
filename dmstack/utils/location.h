#ifndef LOCATION_H
#define LOCATION_H

#include <string>

class Location
{
public:
    Location(std::string file, std::string function, bool is_exe = false)
        : file_(file), function_(function), is_exe_(is_exe) { }
    ~Location() = default;

    bool            is_exe_;
    std::string     file_;
    std::string     function_;
    std::string     filename_;
    unsigned int    lineno_;
};

#endif