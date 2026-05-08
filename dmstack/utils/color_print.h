#ifndef COLOR_H
#define COLOR_H

#include <cstdio>
#ifndef _WIN32


#include <unistd.h>
#endif // !_WIN32

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_LGREEN  "\x1b[92m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_LYELLOW "\x1b[93m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"


#ifdef _WIN32
#define c_printf(color, fmt, ...)                               \
    fprintf(stdout, fmt, ##__VA_ARGS__)

#else

#define c_printf(color, fmt, ...)                               \
    c_fprintf(stdout, color, fmt, ##__VA_ARGS__)

#define c_fprintf(file, color, fmt, ...)                        \
    do {                                                        \
        if (isatty(fileno(file)))                               \
        {                                                       \
            fprintf(file, color fmt COLOR_RESET, ##__VA_ARGS__);\
        }                                                       \
        else                                                    \
        {                                                       \
            fprintf(file, fmt, ##__VA_ARGS__);                  \
        }                                                       \
        fflush(file);                                           \
    } while(0);
#endif // _WIN32

#endif
