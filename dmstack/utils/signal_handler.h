// signal_handler.h
#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <csignal>
#include <atomic>
#include <vector>

namespace utils
{

extern volatile sig_atomic_t interrupt;
extern thread_local void (*tl_signal_handler)(int);
extern const std::vector<int> fatal_signals;

void default_handler(int sig);
void handler(int sig, siginfo_t* s, void* p); 

void install_fatal_signals();
void set_interrupt(int);

extern const std::vector<int> interrupt_signals;
extern sigset_t interrupt_sigset;

void init_interrupt_signal_set();
void install_interrupt_signals();

}

#endif // SIGNAL_HANDLER_H
