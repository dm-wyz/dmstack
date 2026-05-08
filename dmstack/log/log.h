#ifndef LOG_H
#define LOG_H
#ifndef _WIN32

#include <sys/time.h>
#include <unistd.h>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <unordered_map>
#include <string>
#include "config.h"


#define TLBUF_SIZE      1024
#define FILE_NAME       (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#define FMT_BEGIN       "\r[%04d-%02d-%02d %02d:%02d:%02d.%06ld] %s [%s@%s:%d] [%ld] "
#define FMT_END         "\n"
#define FMT(fmt)        FMT_BEGIN fmt FMT_END

namespace Log
{
    struct LogLevel
    {
        enum LogLevelEnum { DEBUG, INFO, WARN, ERROR };
        using LogMap = std::unordered_map<std::string, LogLevelEnum>;

        static inline LogLevelEnum log_level_from_str(const char* str)
        {
            static const LogMap log_map = {
                    {"DEBUG", DEBUG},
                    {"INFO", INFO},
                    {"WARN", WARN},
                    {"ERROR", ERROR}
            };
            auto it = log_map.find(str);
            return (it != log_map.end()) ? it->second : INFO;
        }
    };

    extern thread_local char        tl_log_buf[TLBUF_SIZE];
    extern LogLevel::LogLevelEnum   g_log_level;

    template<typename ... Args>
    static inline void print_log(
        const char*                 log_level, 
        const char*                 function, 
        const char*                 filename, 
        int                         line, 
        const char*                 fmt, 
        Args && ...                 args
    )
    {
        // 获取当前时间
        struct timeval tv;
        struct tm tm_info;

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm_info);

        // 获取进程 ID
        pid_t tid = getpid();
        int log_len = snprintf(tl_log_buf, TLBUF_SIZE, fmt,
                        tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
                        tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec, tv.tv_usec,
                        log_level, function, filename, line, tid, args...);

        // 防止越界
        if (log_len >= TLBUF_SIZE)
        {
            log_len = TLBUF_SIZE;
            tl_log_buf[TLBUF_SIZE - 1] = '\n';

        }

        // 打印到标准错误流
        fprintf(stderr, "%.*s", log_len, tl_log_buf);
        fflush(stderr);
    }
}


#define LOG(level, fmt, ...)                                                    \
    do {                                                                        \
        if (Log::LogLevel::LogLevelEnum::level >= config::g_config.log_level)   \
        {                                                                       \
            Log::print_log(#level, __FUNCTION__, FILE_NAME, __LINE__, FMT(fmt), \
                    ##__VA_ARGS__);                                             \
        }                                                                       \
    } while (0)

#define LOG_LEVEL(str) Log::LogLevel::log_level_from_str(str)

#else

// win 平台不打印日志
#define LOG(level, fmt, ...)
#define LOG_LEVEL(str) 0
#endif // !_WIN32

#endif
