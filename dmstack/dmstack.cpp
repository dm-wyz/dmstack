
#include "dmstack.h"

int main(int argc, char** argv)
{
    config::get_options(argc, argv);
    // parse input and reformat according to the selected options 
    if (!config::g_config.input.empty())
    {
        Output output;
        output.genResult(config::g_config.input);
        return 0;
    }

#ifndef _WIN32
    int rc              = -1;
    int64_t start_ts    = utils::current_time();
    int task_pid        = config::g_config.pid;
    bool thread_only    = static_cast<bool>(config::g_config.thread_only);

    // init signal
    utils::init_interrupt_signal_set();

    //Semaphore sem1, sem2;
    std::string shared_name = "/dmstack_shm" + std::to_string(getpid());
    LOG(INFO, "shared_name: %s", shared_name.c_str());

    // get thread belong to <pid>
    auto thread_map = ThreadInfo::getThreads(task_pid, thread_only);
    if (thread_map.size() == 0)
    {
        LOG(ERROR, "Invaild process id input: %d", task_pid);
        return 0;
    }

    // disable interrupts while main proc waiting
    sigprocmask(SIG_BLOCK, &utils::interrupt_sigset, NULL);
    int coreprocess_pid = fork();
    if (coreprocess_pid != 0) // Parent Process
    {
        LOG(DEBUG, "wait coreprocess get Backtrace...");
        int status;
        int w_pid = wait(&status);

        // Remove signal mask
        sigprocmask(SIG_UNBLOCK, &utils::interrupt_sigset, NULL);
        utils::install_fatal_signals();

        if (-1 == w_pid)
        {
            rc = errno;
            LOG(ERROR, "wait failed, err: %d, errmsg: %s", errno, strerror(errno));
        }
        else
        {
            if (WIFEXITED(status))
            {
                rc = WEXITSTATUS(status);
                if (rc)
                {
                    LOG(WARN, "coreprocess exit with err %d", rc);
                }
            }
            else if (WIFSIGNALED(status))
            {
                LOG(ERROR, "coreprocess killed by signal %d", WTERMSIG(status));
                //exit(1);
            }
            else
            {
                LOG(ERROR, "unhandled status: %d", status);
                //exit(1);
            }
        }

        // generate result and print
        if (0 == rc && !config::g_config.no_parse)
        {
            int64_t detach_ts = utils::current_time();
            try
            {
                // load shared memory
                SharedMemory share(shared_name, true);
                if (share.open() == nullptr)
                {
                    LOG(ERROR, "fail to read shared memory");
                    exit(1);
                }
                // read backtraces from shared memory
                BackTraces backtraces;
                backtraces.deserializeFromBuffer(static_cast<char*>(share.data()), share.size());
                Output output;
                output.genResult(backtraces, thread_map);
            }
            catch(...)
            {
                LOG(ERROR, "parse addrs error");
            }

            LOG(INFO, "parse addrs finish, cost(ms): %f", (utils::current_time() - detach_ts) / 1000.0);
        }

        // warn if stopped
        for (auto& t : thread_map)
        {
            if (ThreadInfo(t.first).isPidStopped())
            {
                LOG(WARN, "process %d is still stopped", t.first);
            }
        }
        LOG(INFO, "exit, cost(ms): %f", (utils::current_time() - start_ts) / 1000.0);
    }
    else
    {
        LOG(DEBUG, "Start coreprocess");
        // Child Process
        sigprocmask(SIG_UNBLOCK, &utils::interrupt_sigset, NULL);
        utils::install_interrupt_signals();

        // Capture thread backtrace by libunwind
        BackTraces backtraces(thread_map);
        rc = backtraces.getErrorCode();
        if (0 == rc)
        {
            LOG(INFO, "all traces detached, task_cnt: %d, cost(ms): %f",
                backtraces.getBackTraces().size(),
                (utils::current_time() - start_ts) / 1000.0);

            if (config::g_config.no_parse)
            {
                Output output;
                output.genResultWithNoparse(backtraces, thread_map);
                return rc;
            }

            // write backtrace in shared memory
            SharedMemory share(shared_name);
            size_t bts_size = backtraces.serializedSize();
            char* buffer = static_cast<char*>(share.allocate(bts_size));
            if (buffer != nullptr)
            {
                backtraces.serializeToBuffer(buffer, bts_size);
            }
            else
            {
                rc = -1;
                LOG(ERROR, "shared memory alloc failed");
            }
        }
    }
    return rc;
#endif // !_WIN32
}


