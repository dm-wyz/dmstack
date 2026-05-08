#ifndef DMSTACK_H
#define DMSTACK_H


#include <iostream>
#include <map>
//#include <cerrno>      // For errno

#ifndef _WIN32
#include <sys/wait.h>
#include "utils/shared_memory.h"
#include "utils/signal_handler.h"
#include "unwind/backtraces.h"
#include "bfd/bfd_cache.h"
#endif // !_WIN32

#include "config.h"
#include "log/log.h"

#include "utils/util.h"
#include "utils/output.h"
#include "utils/color_print.h"

#endif // !DMSTACK_H
