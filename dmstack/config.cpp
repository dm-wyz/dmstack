#include "config.h"
#include "log/log.h"

#include <cstdio>
#ifndef _WIN32
#include <getopt.h>
#endif // !_WIN32

namespace config
{
Config g_config;

void show_version()
{
    printf("dmstack (1.1.1)\n");
    printf("REVISION: %s\n", REVISION);
    printf("BUILD_TIME: %s %s\n", __DATE__, __TIME__);
}

void usage_exit()
{
#ifndef _WIN32


    printf("Usage: \n");
    printf(" dmstack [option(s)] [pid]\n\n");

    printf("Examples: \n");
    printf(" # Capture and parse all thread stacks of process $pid\n");
    printf(" dmstack $pid\n\n");
    printf(" # Read stack traces from stack.txt and reformat according to the selected options\n");
    printf(" dmstack --input stack.txt\n");

    printf("\nOptions: \n");
    //printf("      --format <expanded|compact>    Choose the output style. [default: expanded]\n");
    //printf("                                     expanded  Multi-line, human-readable. One frame per line\n");
    //printf("                                               with a #index (#0 is the top-most frame):\n");
    //printf("                                                   Thread(tname-tid)\n");
    //printf("                                                   #0  0x.. in f0 from ..\n");
    //printf("                                                   #1  0x.. in f1 from ..\n");
    //printf("                                                   #2  0x.. in f2 from ..\n");
    //printf("                                                   #3  0x.. in f3 from ..\n");
    //printf("                                     compact   Single-line, machine-friendly. One thread per line,\n");
    //printf("                                               frames joined by commas; same order as #0,#1,...:\n");
    //printf("                                                   Thread(tname-tid) f0,f1,f2,...\n\n");
    printf("  -f, --format-compact               Shorthand for compact output. One thread per line, frames joined by commas.\n");
    printf("  -l, --log-level <LEVEL>            Log level, LEVEL=DEBUG|INFO|WARN|ERROR\n");
    printf("  -v, --version                      Show version information.\n");
    printf("      --help                         Show help information.\n");

    //printf("\nInput / output: \n");
    printf("      --input <FILE>                 Specify the input file. [default: null]\n");
    printf("                                     This input file must contain raw stack traces captured \n");
    printf("                                     without extra options.(using command \"dmstack <pid>\")\n");
    //printf("      --output <FILE>                Specify where to write the formatted stack traces.\n");
    //printf("                                     Provide a file path to write the output to that file; use \" - \" to\n");
    //printf("                                     write to stdout (the terminal).\n\n");

    //printf("\nAggregation:\n");
    printf("  -a, --aggregate                    Group identical backtraces and output one entry per group\n");
    printf("                                     with occurrence counts.\n");
    printf("  -r, --reverse                      Reverse the aggregation sort order (ascending <--> descending).\n");
    printf("                                     Applies only when --aggregate is active.\n");
    //printf("      --stats                        Emit parsing/aggregation stats (frames, threads, groups, dropped)\n");
    //printf("                                     to stderr.\n");

    //printf("\nSymbolization:\n");
    printf("  -b, --binary-path <PATH>           Search path for binaries/executables.\n");
    printf("  -s, --symbol-path <PATH>           Search path for separate debug symbol files.\n");
    printf("      --search-system-symbols        Also search system debug locations (/usr/lib/.build-id,/usr/lib/debug).\n");

    //printf("\nParsing & filtering:\n");
    printf("  -t, --thread-only                  Process a single thread only (the selected/current one).\n");
    printf("  -i, --min-depth <N>                Output only threads whose stack depth ≥ N. [Default: 0]\n");
    printf("  -c, --priority-threads <PATTERNS>  List of space-separated patterns; matching threads are printed first.\n");
    //printf("      --frames <N>                   Output only the first N frames; 0 = unlimited. [Default: 0]\n");
    //printf("      --skip <N>                     Skip the top N frames. [Default: 0]\n");
    //printf("      --strip-prefix <PATH>          Strip this path prefix when printing file paths.\n");
    //printf("      --keep-thread <REGEX>          Keep only threads whose name/ID matches (regex).\n");
    //printf("      --exclude-thread <REGEX>       Drop threads whose name/ID matches (regex).\n");
    //printf("      --keep-path <REGEX>            Keep frames whose library/path matches (regex).\n");
    //printf("      --exclude-path <REGEX>         Drop frames whose library/path matches (regex).\n");
    //printf("      --merge-adjacent               Coalesce adjacent duplicate frames into “×N”.\n");

    //printf("\nPrinting details:\n");
    printf("  -n, --no-parse                     Print addresses only (no symbols/paths/files).\n");
    printf("  -h, --hide-thread <0|1|2>          Hide thread header fields(thread name and id), where 0=none, 1=id, 2=all.\n");
    printf("                                     [Default: 0]\n\n");
    //printf("      --hide-addr                      Hide addresses. Applies only when --format expanded is active.\n");
    //printf("      --hide-file                      Hide source file/line.Applies only when --format expanded is active.\n");


    //printf("  -l, --log_level=[DEBUG|INFO|WARN|ERROR]              : Log level\n");
    //printf("  -n, --no_parse                                       : Output address only\n");
    //printf("  -a, --aggregate                                            : Aggregate backtrace\n");
    //printf("  -b, --binary_path=next_path                          : Binary next_path\n");
    //printf("  -s, --symbol_path=next_path                          : Symbol next_path\n");
    //printf("  -t, --thread_only                                    : Process single thread only\n");
    //printf("  -f, --format_compact                                  : Output function name only\n");
    //printf("  -h, --hide_thread=num                                : Output without thread id and name, optional value:0,1,2\n");
    //printf("  -i, --ingore_level=num                               : Output thread stack excceed num only\n");
    //printf("  -c, --priority-threads                                    : Output cared thread first, input like \"str1 str2\"\n");
    //printf("  -r, --reverse                                        : Change ascending/descending of the aggregate\n");
    //printf("      --search_system_symbol                           : Search symbols in /usr/lib/debug\n");
    //printf("      --input <FILE>                                   : Input file. [default: null]\n");
    //printf("      --output <FILE>                                  : Output file. [default: console]\n");
    //printf("  -v, --version                                        : Output version number\n");


#else

    printf("Usage: \n");
    printf(" dmstack [option(s)] [pid]\n\n");

    printf("Examples: \n");
    printf(" # Read stack traces from stack.txt and reformat according to the selected options\n");
    printf(" dmstack --input stack.txt\n\n");

    printf("Options: \n");
    printf("      --input <FILE>                 Specify the input file. [default: null]\n");
    printf("                                     This input file must contain raw stack traces captured \n");
    printf("                                     without extra options.(using command \"dmstack <pid>\")\n");
#endif // !_WIN32
    exit(1);
}

#ifdef _WIN32
enum
{
    no_argument = 0,
    required_argument = 1,
    optional_argument = 2
};

struct option
{
    const char* name;
    int         has_arg;
    int*        flag;
    int         val;
};

char* optarg = NULL;
int   optind = 1;
int   opterr = 1;
int   optopt = '?';

// 用于处理 -abc 这样的短选项簇
static char* s_nextchar = NULL;

// 查询短选项是否需要参数
static int shortopt_need_arg(const char* optstring, int c)
{
    if (!optstring)
    {
        return 0;
    }

    for (const char* p = optstring; *p; ++p)
    {
        if (*p == c)
        {
            return (p[1] == ':');
        }
    }

    return 0;
}

// 处理长选项：返回 <0 表示未处理；返回值同 getopt_long 规范
static int handle_longopt(int argc, char* const argv[], 
    const char* optstring, const struct option* longopts, int* longindex)
{
    const char* arg = argv[optind];

    if (strncmp(arg, "--", 2) != 0)
    {
        return -2;
    }

    const char* name    = arg + 2;
    const char* eq      = strchr(name, '=');
    char*       value   = NULL;

    // 精确匹配（不支持前缀缩写，避免歧义）
    int         match   = -1;
    if (!longopts)
    { 
        optind++; 
        return '?';
    }

    size_t name_len = eq ? (size_t)(eq - name) : strlen(name);

    for (int i = 0; longopts[i].name; ++i)
    {
        if (strlen(longopts[i].name) == name_len &&
            strncmp(longopts[i].name, name, name_len) == 0)
        {
            match = i;
            break;
        }
    }

    if (match < 0)
    {
        // 未知长选项
        optopt = 0;
        optind++;
        return '?';
    }

    if (eq)
    {
        value = (char*)eq + 1;
    }

    const struct option* o = &longopts[match];

    if (longindex)
    {
        *longindex = match;
    }

    // 处理参数需求
    if (o->has_arg == required_argument)
    {
        if (value)
        {
            optarg = value;
        }
        else if (optind + 1 < argc)
        {
            optarg = argv[optind + 1];
            optind++;
        }
        else
        {
            optopt = o->val;
            optind++;
            return '?';
        }
    }
    else if (o->has_arg == optional_argument)
    {
        optarg = value; // 可能为 NULL
    }
    else
    {
        // no_argument
        if (value)
        {
            // 不应带 =value，按错误处理
            optopt = o->val;
            optind++;
            return '?';
        }
        optarg = NULL;
    }

    optind++; // 消费当前 --name[=value]

    if (o->flag)
    {
        *(o->flag) = o->val;
        return 0; // GNU 语义：flag 模式返回 0
    }
    return o->val;
}

int getopt_long(int argc, char* const argv[], const char* optstring,
    const struct option* longopts, int* longindex)
{
    optarg = NULL;

    // 如果当前没有在处理短选项簇，拉取下一个 argv
    if (!s_nextchar || *s_nextchar == '\0')
    {
        if (optind >= argc)
        {
            return -1;
        }

        const char* arg = argv[optind];

        if (strcmp(arg, "--") == 0)
        {
            // 显式终止
            optind++;
            return -1;
        }

        if (arg[0] != '-' || arg[1] == '\0')
        {
            // 非选项，停止解析（保持与 GNU 行为一致）
            return -1;
        }

        if (arg[0] == '-' && arg[1] == '-')
        {
            // 长选项
            return handle_longopt(argc, argv, optstring, longopts, longindex);
        }

        // 短选项簇
        s_nextchar = (char*)arg + 1;
    }

    // 处理短选项（可能是簇的一部分）
    int c = (unsigned char)*s_nextchar++;
    int need_arg = shortopt_need_arg(optstring, c);

    if (!strchr(optstring ? optstring : "", c))
    {
        // 未知短选项
        optopt = c;
        if (!*s_nextchar)
        { 
            optind++; 
            s_nextchar = NULL;
        }

        return '?';
    }

    if (need_arg)
    {
        if (*s_nextchar)
        {
            // -lINFO
            optarg = s_nextchar;
            optind++;
            s_nextchar = NULL;
        }
        else if (optind + 1 < argc)
        {
            // -l INFO
            optarg = argv[optind + 1];
            optind += 2;
            s_nextchar = NULL;
        }
        else
        {
            // 参数缺失
            optopt = c;
            optind++;
            s_nextchar = NULL;
            return '?';
        }
    }
    else
    {
        // 无参数
        if (!*s_nextchar)
        {
            optind++;
            s_nextchar = NULL;
        }
    }
    return c;
}
#endif // _WIN32

void get_options(int argc, char** argv)
{
    int c;
    struct option long_options[] =
        {
            {"?",                       no_argument,        NULL, '?'},
            {"version",                 no_argument,        NULL, 'v'},
            {"aggregate",               no_argument,        NULL, 'a'},
            {"no-parse",                no_argument,        NULL, 'n'},
            {"no-lineno",               no_argument,        NULL, 'o'},
            {"format-compact",          no_argument,        NULL, 'f'},
            {"reverse",                 no_argument,        NULL, 'r'},
            {"hide-thread",             required_argument,  NULL, 'h'},
            {"log-level",               required_argument,  NULL, 'l'},
            {"binary-path",             required_argument,  NULL, 'b'},
            {"symbol-path",             required_argument,  NULL, 's'},
            {"priority-threads",        required_argument,  NULL, 'c'},
            {"min-depth",               required_argument,  NULL, 'i'},
            {"input",                   required_argument,  NULL, OPT_INPUT_PATH},
            {"output",                  required_argument,  NULL, OPT_OUTPUT_PATH},
            {"bfd-threads",             required_argument,  NULL, OPT_BFD_THREADS},
            {"search-system-symbols",   no_argument,        NULL, OPT_SEARCH_SYMBOL},
            {NULL,                      0,                  NULL, 0}
        };
    g_config = { 0 };
    g_config.log_level = 3; // ERROR

    if (argc == 1)
    {
        usage_exit();
    }

    while ((c = getopt_long(argc, argv, "?onavtfrh:l:b:s:c:i:", long_options, (int*)0))
        != EOF)
    {
        switch (c)
        {
        case '?':
        {
            usage_exit();
            break;
        }
        case 'l':
        {
            g_config.log_level = LOG_LEVEL(optarg);
            break;
        }
        case OPT_SEARCH_SYMBOL:
        {
            g_config.search_symbols = true;
            break;
        }
        case OPT_BFD_THREADS:
        {
            g_config.bfd_threads = atoi(optarg);
            LOG(INFO, "input bfd_threads: %d", g_config.bfd_threads);
            break;
        }
        case 'r':
        {
            g_config.reverse = true;
            break;
        }
        case 'f':
        {
            g_config.format_compact = true;
            break;
        }
        case 'h':
        {
            g_config.hide_thread = atoi(optarg);
            LOG(INFO, "input hide_thread: %d", g_config.hide_thread);
            break;
        }
        case 'b':
        {
            g_config.binary_path = optarg;
            LOG(INFO, "input binary-path: %s", g_config.binary_path.c_str());
            break;
        }
        case 's':
        {
            g_config.symbol_path = optarg;
            LOG(INFO, "input symbol-path: %s", g_config.symbol_path.c_str());
            break;
        }
        case OPT_INPUT_PATH:
        {
            g_config.input = optarg;
            LOG(INFO, "input input: %s", g_config.input.c_str());
            break;
        }
        case OPT_OUTPUT_PATH:
        {
            g_config.output = optarg;
            LOG(INFO, "input output: %s", g_config.output.c_str());
            break;
        }
        case 'i':
        {
            g_config.min_depth = atoi(optarg);
            LOG(INFO, "input min-depth: %d", g_config.min_depth);
            break;
        }
        case 'c':
        {
            g_config.tnames = optarg;
            LOG(INFO, "input priority-threads: %s", g_config.tnames.c_str());
            break;
        }
        case 'n':
        {
            g_config.no_parse = true;
            break;
        }
        case 'a':
        {
            g_config.aggregate = true;
            break;
        }
        case 'o':
        {
            g_config.no_lineno = true;
            break;
        }
        case 'v':
        {
            show_version();
            exit(0);
            break;
        }
        case 't':
        {
            g_config.thread_only = true;
            break;
        }
        default:
        {
            usage_exit();
            break;
        }
        }
    }

    if (argc > optind)
    {
        g_config.pid = atoi(argv[optind]);
        LOG(INFO, "input pid: %d", g_config.pid);
    }
}
}
