#ifndef OUTPUT_H
#define OUTPUT_H

#include <algorithm>
#include <fstream>
#include <sstream>
#include "config.h"
#include "utils/color_print.h"
#include "utils/location.h"
#ifndef _WIN32
#include "utils/shared_memory.h"

#include "bfd/bfd_cache.h"
#endif // !_WIN32
#include "unwind/backtraces.h"

class Output
{
public:
    Output() :config_(config::g_config) {}

    void genResultWithNoparse(BackTraces& backtraces, std::unordered_map<int, std::string>& thread_map) const
    {
        if (!config_.no_parse)
        {
            return;
        }

        auto& bts = backtraces.getBackTraces();
        for (auto& bt : bts)
        {
            // ignore thread while bt.size <= min_depth
            if (bt.addrs().size() <= config_.min_depth) continue;

            c_printf(COLOR_YELLOW, "Thread (%s-%d), bt:", thread_map[bt.tid()].c_str(), bt.tid());
            for (auto& addr : bt.addrs())
            {
                unsigned long v = addr;
                /*auto pt_load = bfd_cache.find_pt_load(addr);
                if (pt_load && pt_load->is_exe_) {
                    v = pt_load->addr2offset(addr);
                }*/
                c_printf(COLOR_CYAN, " 0x%lx", v);
            }
            c_printf(COLOR_CYAN, "\n");
        }
    }

    template<typename Addrs>
    void print_stack_frames(Addrs& addrs, bool with_function_only = false, bool with_frame_no = true)
    {
        int frame = 0;
        for (auto&& addr : addrs)
        {
            auto it = loc_cache_.end();
            if (with_frame_no)
            {
                c_printf(COLOR_RESET, "#%-4d ", frame);
            }
            frame++;
            if ((it = loc_cache_.find(addr)) != loc_cache_.end())
            {
                auto& loc = it->second;
                if (with_function_only)
                {
                    if (!loc.is_exe_)
                    {
                        c_printf(COLOR_CYAN, "%s%s", loc.function_.c_str(),
                            frame == addrs.size() ? "" : ",");
                    }
                    else
                    {
                        c_printf(COLOR_LGREEN, "%s%s", loc.function_.c_str(),
                            frame == addrs.size() ? "" : ",");
                    }
                }
                else
                {
                    constexpr bool fn_valid = false;
                    //bool fn_valid =
                    //    loc.filename_.length() > 0 &&
                    //    0 != loc.filename_.compare("0") &&
                    //    0 != loc.filename_.compare("(null)") &&
                    //    loc.lineno_ > 0;
                    if (fn_valid)
                    {
                        c_printf(COLOR_CYAN, "0x%016lx in %s at %s:%d\n", addr, loc.function_.c_str(),
                            loc.filename_.c_str(), loc.lineno_);
                    }
                    else
                    {
                        c_printf(COLOR_CYAN,    "0x%016lx", addr);
                        c_printf(COLOR_RESET,   " in");
                        c_printf(COLOR_LYELLOW, " %s", loc.function_.c_str());
                        c_printf(COLOR_RESET,   " from");
                        c_printf(COLOR_LGREEN,  " %s\n", loc.file_.c_str());
                        //c_printf(COLOR_CYAN, "0x%016lx in %s from %s\n", addr, loc.function_.c_str(),
                        //    loc.file_.c_str());
                    }
                }
            }
            else
            {
                c_printf(COLOR_CYAN, "0x%016lx in ???\n", addr);
            }
        }
        if (with_function_only)
        {
            c_printf(COLOR_CYAN, "\n");
        }
    }

    std::vector<std::string> splitString(const std::string& str)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;

        while (iss >> token)
        {
            if (!token.empty())
            {
                tokens.push_back(token);
            }
        }

        return tokens;
    }

    
    void genResult(std::vector<BackTrace>& _bts, std::unordered_map<int, std::string>& thread_map_)
    {
        if (config_.aggregate)
        {
            struct Value
            {
                std::vector<int>         tids_;
                std::vector<std::string> tnames_;
            };

            std::map<BackTrace, Value> bt_map;
            for (auto&& bt : _bts)
            {
                auto it = bt_map.find(bt);
                if (it == bt_map.end())
                {
                    it = bt_map.insert({ bt, Value() }).first;
                }
                it->second.tids_.push_back(bt.tid_);
                it->second.tnames_.push_back(thread_map_[bt.tid_]);
            }
            using Iterator = decltype(bt_map)::iterator;
            std::vector<Iterator> sorted_bts;
            for (auto it = bt_map.begin(); it != bt_map.end(); it++)
            {
                sorted_bts.push_back(it);
            }
            if (config_.reverse)
            {
                std::sort(sorted_bts.begin(), sorted_bts.end(),
                    [](Iterator& l, Iterator& r)
                    {
                        return l->second.tids_.size() < r->second.tids_.size();
                    }
                );
            }
            else
            {
                std::sort(sorted_bts.begin(), sorted_bts.end(), 
                    [](Iterator& l, Iterator& r)
                    {
                        return l->second.tids_.size() > r->second.tids_.size();
                    }
                );
            }

            for (auto&& it : sorted_bts)
            {
                auto& tids = it->second.tids_;
                auto& tnames = it->second.tnames_;

                // ignore thread while bt.size <= min_depth
                if (it->first.addrs().size() <= config_.min_depth)
                    continue;

                c_printf(COLOR_YELLOW, "%lu-Thread%s", tids.size(), tids.size() == 1 ? "" : "s");
                // skip thread id and name
                if (config_.hide_thread <= 1)
                {
                    std::unordered_map<std::string, std::vector<int>> tmp_map;
                    for (int i = 0; i < tids.size(); i++)
                    {
                        tmp_map[tnames[i]].push_back(tids[i]);
                    }

                    c_printf(COLOR_YELLOW, "(");
                    int pair_pos = 0;
                    for (auto& pair : tmp_map)
                    {
                        if (pair_pos != 0) 
                            c_printf(COLOR_YELLOW, ", ");
                        pair_pos++;

                        c_printf(COLOR_YELLOW, "%s", pair.first.c_str());
                        if (config_.hide_thread < 1)
                        {
                            c_printf(COLOR_YELLOW, "-");
                            for (int i = 0; i < pair.second.size(); i++)
                            {
                                // Separator ','
                                if (i != 0) c_printf(COLOR_YELLOW, ",");
                                c_printf(COLOR_YELLOW, "%d", pair.second[i]);
                            }
                        }
                    }
                    c_printf(COLOR_YELLOW, ")");
                }
                c_printf(COLOR_YELLOW, " ");

                // show result in one line
                if (!config_.format_compact) c_printf(COLOR_YELLOW, "\n");

                // print stack frames
                print_stack_frames(it->first.addrs(), config_.format_compact,
                    config_.format_compact ? false: true);
            }
        }
        else
        {
            for (auto&& bt : _bts)
            {
                // ignore thread while bt.size <= min_depth
                if (bt.addrs().size() <= config_.min_depth) continue;

                // skip thread id and name
                if (config_.hide_thread == 0)
                {
                    c_printf(COLOR_YELLOW, "Thread(%s-%d) ", thread_map_[bt.tid()].c_str(), bt.tid());
                }
                else if (config_.hide_thread == 1)
                {
                    c_printf(COLOR_YELLOW, "Thread(%s) ", thread_map_[bt.tid()].c_str());
                }


                // show result in one line
                if (!config_.format_compact) c_printf(COLOR_YELLOW, "\n");

                // print stack frames
                print_stack_frames(bt.addrs(), config_.format_compact,
                    config_.format_compact ? false : true);
            }
        }

    }
#ifndef _WIN32


    void genResult(BackTraces& backtraces, std::unordered_map<int, std::string>& thread_map_)
    {
        // do addr2symbol
        auto&& bts = backtraces.getBackTraces();
        BFDCache bfd_cache(config_.pid);
        if (bfd_cache.getErrorCode())
        {
            LOG(ERROR, "bfd init error");
            return;
        }

        for (auto& it : bts)
        {
            for (auto& addr : it.addrs())
            {
                loc_cache_.emplace(addr, bfd_cache.addr2symbol(addr));
            }
        }

        auto filterThreads = [&thread_map_](
            const std::vector<BackTrace>& threads,
            const std::vector<std::string>& splitString )
        {
            std::vector<BackTrace> included;
            std::vector<BackTrace> excluded;

            std::copy_if(threads.begin(), threads.end(), std::back_inserter(included),
                [&](const BackTrace& bt) {
                return std::any_of(splitString.begin(), splitString.end(),
                    [&](const std::string& sub) { return thread_map_[bt.tid()].find(sub) != std::string::npos; });
            });

            std::copy_if(threads.begin(), threads.end(), std::back_inserter(excluded),
                [&](const BackTrace& bt) {
                return std::none_of(splitString.begin(), splitString.end(),
                    [&](const std::string& sub) { return thread_map_[bt.tid()].find(sub) != std::string::npos; });
            });

            return std::make_pair(included, excluded);
        };

        auto split = splitString(config_.tnames);
        auto group = filterThreads(bts, split);

        auto& included = group.first;
        auto& excluded = group.second;

        if (included.size() > 0)
        {
            genResult(included, thread_map_);
        }

        if (excluded.size() > 0)
        {
            genResult(excluded, thread_map_);
        }
    }
#endif // !_WIN32

    void genResult(std::string input_file)
    {
        std::vector<BackTrace>                  backtraces;
        std::unique_ptr<BackTrace>              current;
        std::unordered_map<int, std::string>    thread_map;
        std::ifstream                           file(input_file.c_str());
        std::string                             line;

        // Open input file
        if (!file.is_open())
        {
            LOG(ERROR, "fail to open %s", input_file.c_str());
            return;
        }
        //DEFER(file.close());

        while (std::getline(file, line))
        {
            // Thread header
            char        thread_name[256];
            int         thread_id = 0;
            // Backtrace info
            int         frame_no;
            int64_t     address;
            char        function[256];
            char        file[1024];

            // Backtrace format like:
            // #0    0x00007f43edfcad71 in wait from /usr/lib/x86_64-linux-gnu/libc.so.6
            int n = sscanf(line.c_str(), "#%d 0x%lx in %s from %s",
                &frame_no, &address, function, file);
            if (n == 4)
            {
                current->addAddr(address);
                loc_cache_.emplace(address, Location(file, function));

                // Enter the next line
                continue;
            }


            // Thread header format like: Thread(thread_name-1235)
            if (sscanf(line.c_str(), "Thread(%255[^-]-%d)", thread_name, &thread_id) == 2)
            {
                if (current) {
                    backtraces.emplace_back(*current);
                    current.reset();
                }
                current = std::make_unique<BackTrace>(thread_id);
                thread_map[thread_id] = thread_name;

                // Enter the next line
                continue;
            }

            // 
            LOG(ERROR, "Invalid format.");
            return;
        }

        // End of file, emplace the last into backtraces
        if (current)
        {
            backtraces.emplace_back(*current);
            current.reset();
        }

        if (backtraces.size() > 0)
        {
            genResult(backtraces, thread_map);
        }
    }

    config::Config              config_;
    std::map<int64_t, Location> loc_cache_;
};

#endif // !OUTPUT_H
