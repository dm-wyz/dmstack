#include "signal_handler.h"

#include "log/log.h"

namespace utils
{

volatile sig_atomic_t interrupt = 0;

void default_handler(int sig)
{
    std::signal(sig, SIG_DFL);
    std::raise(sig);
}

thread_local void (*tl_signal_handler)(int) = default_handler;
const std::vector<int> fatal_signals = { SIGABRT, SIGBUS, SIGFPE, SIGSEGV };

void handler(int sig, siginfo_t* s, void* p)
{
    if (tl_signal_handler != nullptr)
    {
        tl_signal_handler(sig);
    }
}

void install_fatal_signals()
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO | SA_RESTART | SA_NODEFER | SA_ONSTACK;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);

    for (int sig : fatal_signals)
    {
        if (sigaction(sig, &sa, nullptr) == -1)
        {
            LOG(ERROR, "Failed to install signal handler for signal %d", sig);
            exit(-1);
        }
    }
}

void set_interrupt(int)
{
    interrupt = true;
}

const std::vector<int> interrupt_signals = { SIGHUP, SIGINT, SIGTERM };
sigset_t interrupt_sigset;

void init_interrupt_signal_set()
{
    sigemptyset(&interrupt_sigset);
    for (int sig : interrupt_signals)
    {
        sigaddset(&interrupt_sigset, sig);
    }
}

void install_interrupt_signals()
{
    for (int sig : interrupt_signals)
    {
        std::signal(sig, set_interrupt);
    }
}

}
