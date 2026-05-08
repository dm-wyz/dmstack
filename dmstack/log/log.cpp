#include "log.h"

namespace Log
{

// thread_local tl_log_buf
thread_local char tl_log_buf[TLBUF_SIZE];

}
