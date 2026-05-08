#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
namespace config
{
struct Config
{
    int             pid;
    int             min_depth;
    int             bfd_threads;
    bool            format_compact;
    bool            aggregate;
    bool            reverse;
    bool            no_parse;
    bool            no_lineno;
    int             hide_thread;
    int             log_level;
    bool            thread_only;
    bool            search_symbols;
    std::string     symbol_path;
    std::string     binary_path;
    std::string     exec_path;
    std::string     tnames;
    std::string     input;
    std::string     output;
};

enum
{
    OPT_SEARCH_SYMBOL = 1000,
    OPT_BFD_THREADS,
    OPT_INPUT_PATH,
    OPT_OUTPUT_PATH
};

extern Config g_config;

void show_version();
void usage_exit();
void get_options(int argc, char** argv);
}
#endif // !CONFIG_H
