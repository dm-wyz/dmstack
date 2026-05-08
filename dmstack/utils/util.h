#ifndef UTIL_H
#define UTIL_H

#ifndef _WIN32
#include <sys/time.h>  // For gettimeofday() and struct timeval
#include <cerrno>      // For errno

#include "log/log.h"
#endif

namespace utils
{

static inline int64_t current_time()
{
#ifdef _WIN32
    return 0;
#else

    int err_ret = 0;
    struct timeval t;
    if (__builtin_expect((err_ret = gettimeofday(&t, nullptr)) < 0, 0))
    {
        LOG(ERROR, "gettimeofday error, err_ret: %d, errno: %d", err_ret, errno);
        exit(1);
    }
    return (static_cast<int64_t>(t.tv_sec) * 1000000L +
        static_cast<int64_t>(t.tv_usec));

#endif // _WIN32
}

}
#endif
