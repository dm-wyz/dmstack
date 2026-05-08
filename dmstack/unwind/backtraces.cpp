#include "unwind/backtraces.h"
#include "utils/signal_handler.h"
#include "utils/defer.h"
#include "log/log.h"

#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>

#include <fstream>

void wait_info(int tid, int task_idx, int total_task, long long wait_count)
{
    char line[128];

    // 避免打印过于频繁
    if (wait_count % 1000 == 0)
    {
        char bar[21];
        int fill;
        int i;

        int percent = (total_task > 0) ? (int)((long long)task_idx * 100 / total_task) : 0;
        if (percent < 0)
            percent = 0;
        if (percent > 100)
            percent = 100;

        fill = percent * 20 / 100;
        for (i = 0; i < 20; i++)
            bar[i] = (i < fill) ? '#' : '-';
        bar[20] = '\0';

        snprintf(line, sizeof(line),
            "T%-5d [%-20s] %3d%% %4d/%4d",
            tid, bar, percent, task_idx, total_task);

        // 固定整行输出宽度，
        fprintf(stderr, "\r%-80s\r", line);
        fflush(stderr);
    }
}

BackTraces::BackTraces(std::unordered_map<int, std::string>& tasks): error_code_(0)
{
    int rc = 0;
    int task_idx = 0;
    int total_task = tasks.size();
    int w_pid = 0;
    int status = 0;
    int count = 0;

    bts_.reserve(total_task);

    do {
        unw_addr_space_t addr_space = unw_create_addr_space(&_UPT_accessors, 0);
        if (!addr_space)
        {
            rc = -1;
            LOG(ERROR, "unw_create_addr_space failed");
            break;
        }
        DEFER(if (addr_space) unw_destroy_addr_space(addr_space));
        unw_set_caching_policy(addr_space, UNW_CACHE_GLOBAL);

        for (auto &t: tasks)
        {
            // init local bt
            BackTrace bt(t.first);

            // reset wait info
            rc = 0;
            w_pid = 0;
            status = 0;
            count = 0;
            task_idx++;

            // break the loop while interrupt
            if (utils::interrupt)
            {
                break;
            }

            // attach
            rc = ptrace(PTRACE_ATTACH, t.first);
            if (-1 == rc)
            {
                if (errno != ESRCH)
                {
                    LOG(ERROR, "ptrace attach failed, tid: %d, err: %d, errmsg: %s",
                        t.first, errno, strerror(errno));
                }
                continue;
            }
            DEFER(ptrace(PTRACE_DETACH, t.first, 0, 0));

            // wait stop
            while (1)
            {
                w_pid = wait4(t.first, &status, __WALL | WNOHANG, NULL);

                // no status change
                if (0 == w_pid)
                {
                    usleep(20);
                    wait_info(t.first, task_idx, total_task, count++);
                    continue;
                }

                if (-1 == w_pid)
                {
                    // Interrupted system call, should retry
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    rc = errno;
                    break;
                }
                else if (WIFEXITED(status) || WIFSIGNALED(status))
                {
                    // no such process
                    rc = ESRCH;
                    break;
                }
                else if (WIFSTOPPED(status))
                {
                    // successfully stopped
                    rc = 0;
                    break;
                }

            }

            if (rc != 0)
            {
                LOG(ERROR, "wait %d failed, err: %d, errmsg: %s", t.first, rc, strerror(rc));
                continue;
            }

            // unwind backtrace
            struct UPT_info* upt_info = (struct UPT_info*)_UPT_create(t.first);
            if (!upt_info)
            {
                LOG(WARN, "unw_create_addr_space failed");
                rc = -1;
                continue;
            }
            DEFER(_UPT_destroy(upt_info));

            unw_cursor_t c;
            unw_init_remote(&c, addr_space, upt_info);
            do {
                unw_word_t uip;
                if ((rc = unw_get_reg(&c, UNW_REG_IP, &uip)) < 0)
                {
                    LOG(WARN, "get reg failed, err: %d\n", rc);
                    break;
                }
                bt.addAddr(static_cast<int64_t>(uip));
            } while ((rc = unw_step(&c)) > 0);

            if (bt.addrs().size() > 0)
            {
                bts_.push_back(bt);
            }
        }
        if (utils::interrupt) {
            error_code_ = -1;
            LOG(WARN, "interruption occurs, will exit...");
        }
    } while (0);
}

std::unordered_map<int, std::string> ThreadInfo::getThreads(int pid, bool thread_only)
{
    std::unordered_map<int, std::string> thread_map;
    std::string path = "/proc/" + std::to_string(pid) + "/task";

    struct dirent* entry;
    struct stat entry_stat;

    auto getThreadName = [](int tid)
    {
        std::string path = "/proc/" + std::to_string(tid) + "/comm";
        std::ifstream file(path);
        if (!file.is_open())
            return std::string("Unknown");

        std::string name;
        std::getline(file, name);
        file.close();

        return name;
    };

    if (thread_only)
    {
        std::string name = getThreadName(pid);
        thread_map.emplace(pid, name);
        return thread_map;
    }


    DIR* dir = opendir(path.c_str());
    if (!dir)
    {
        LOG(ERROR, "Failed to open directory: %s", path.c_str());
        return thread_map;
    }

    // 遍历 /proc/{pid}/task/ 目录，获取所有线程 TID
    while ((entry = readdir(dir)) != nullptr)
    {
        std::string full_path = path + "/" + entry->d_name;

        if (stat(full_path.c_str(), &entry_stat) == 0 &&
            S_ISDIR(entry_stat.st_mode))
        {
            std::string dirName = entry->d_name;
            if (dirName != "." && dirName != "..")
            {
                int tid = std::stoi(dirName);
                std::string name = getThreadName(tid);
                thread_map.emplace(tid, name);
            }
        }
    }

    closedir(dir);
    return thread_map;
}

bool ThreadInfo::isPidStopped()
{
    FILE* status_file;
    char buf[100];
    bool stopped = false;

    snprintf(buf, sizeof(buf), "/proc/%d/status", (int)tid_);
    status_file = fopen(buf, "r");
    if (status_file != NULL)
    {
        int have_state = 0;
        while (fgets(buf, sizeof(buf), status_file))
        {
            buf[strlen(buf) - 1] = '\0';
            if (strncmp(buf, "State:", 6) == 0)
            {
                have_state = 1;
                break;
            }
        }
        if (have_state && strstr(buf, "T") != NULL)
        {
            LOG(WARN, "process %d %s", tid_, buf);
            stopped = true;
        }
        fclose(status_file);
    }
    return stopped;
}
