#include "bfd/bfd_cache.h"
#include "bfd/bfd_utils.h"

#include "config.h"
#include "log/log.h"
#include "utils/defer.h"

#include <algorithm>
#include <fstream>
#include <functional>
#include <thread>
#include <dirent.h>  // POSIX directory operations
#include <sys/stat.h> // For stat()
#include <unistd.h>   // For chdir() and getcwd()

PTLoad::PTLoad(int64_t start, int64_t end, std::string file, bool is_exe) :
    addr_start_(start), addr_end_(end), file_(file), is_exe_(is_exe)
{
    //if (is_exe_)
    //{
    //    int64_t vaddr;
    //    bfd_utils::check_shlib(file_.c_str(), vaddr);
    //    load_vaddr_ = vaddr;
    //}

    // On x86_64 the default base load address is 0x400000,
    // so most processes' executables' addr_start_ and load_vaddr_ are 0x400000.
    load_vaddr_ = is_exe_ ? start : 0;

}

void PTLoad::createBFDInfo()
{
    info_ = std::make_shared<BFDInfo>(file_, is_exe_);
}
void PTLoad::updateLoadAddr()
{
    // Special case: apply a temporary fix below.
    if (info_->sym_ents_.size() != 0)
    {

        if (info_->sym_ents_.begin()->addr_ >= addr_start_)
        {
            // If the first symbol address reported by BFD is greater than or equal to 'start',
            // (For example  on RHEL6)
            // it means that the runtime stack addresses returned by libunwind
            // already match the symbol addresses from BFD, so no offset adjustment is needed.
            // In that scenario, we set load_vaddr_ to 'start'.
            load_vaddr_ = addr_start_;
        }
        else
        {
            // Otherwise (for example, in our test1 case under this project),
            // the correct load_vaddr_ is 0, not 'start', so we clear it.
            load_vaddr_ = 0;
        }
    }

    LOG(DEBUG, "start: %lx, end: %lx, load: %lx, file:%s",
        addr_start_, addr_end_, load_vaddr_, file_.c_str());
}

BFDInfo::BFDInfo(std::string file, bool is_exe) : file_(std::move(file))
{
    // Open file using bfd
    auto abfd = openBfd(file_);
    if (!abfd)
    {
        error_code_ = -1;
        LOG(WARN, "create pt load failed, file: %s", file_.c_str());
        return;
    }

    std::string load_file = findFileWithSymbols(abfd.get(), file_, false);
    if (load_file.empty())
    {
        error_code_ = -1;
        LOG(WARN, "No symbol in file: %s", file_.c_str());
        return;
    }

    if (file_ != load_file)
    {
        // Open file using bfd
        abfd = openBfd(load_file);
        if (!abfd)
        {
            error_code_ = -1;
            LOG(WARN, "create pt load failed, file: %s", load_file.c_str());
            return;
        }
    }

    loadSymbols(abfd.get());

    load_file_ = load_file;
}

BFDInfo::BfdPtr BFDInfo::openBfd(const std::string& path)
{
    bfd* handle = bfd_openr(path.c_str(), nullptr);
    if (!handle || bfd_check_format(handle, bfd_archive) ||
        !bfd_check_format_matches(handle, bfd_object, nullptr))
    {
        if (handle)
        {
            bfd_close(handle);
        }

        return { nullptr, &bfd_close };
    }
    return { handle, &bfd_close };

}

// Find file with symbol table:
// 
// 1.check_stripped :Yes-> 2.find same file  in binary_path : Yes-> 6.
//      |No                     No V
//      |    3. find debug_link in symbol_path or curr_path : Yes-> 6.
//      |                       No V
//      |    4. find debug_link in /usr/lib/[debug]/.build-id : Yes-> 6.
//      |                       No V
//      |    5. search debug_link in subdir of /usr/lib/debug : Yes-> 6.
//      V                       No V
// 6. load symbol success   7. Error: No symbol
std::string BFDInfo::findFileWithSymbols(bfd* abfd, const std::string& file, bool is_exe)
{
    if (!bfd_utils::check_stripped(file.c_str()))
    {
        return file;
    }

    auto pair_ = splitPath(file);
    std::string current_path_ = pair_.first;
    std::string file_name_ = pair_.second;

    // Process binary_path
    if (!config::g_config.binary_path.empty())
    {
        std::string p = config::g_config.binary_path + file_name_;
        if (!file_name_.empty() &&
            !bfd_utils::check_stripped(p.c_str()))
        {
            return p;
        }
    }

    // Format like "/usr/lib/.build-id/xx/xxxx" or
    //             "/usr/lib/debug/.build-id/xx/xxxx"
    auto buildid = getBuildId(abfd);
    if (!buildid.empty())
    {
        for (auto& base : { "/usr/lib/.build-id/", "/usr/lib/debug/.build-id/" })
        {
            std::string p = base + buildid;
            if (bfd_utils::check_exists_and_is_regular(p.c_str()))
            {
                return p;
            }

            p += ".debug";
            if (bfd_utils::check_exists_and_is_regular(p.c_str()))
            {
                return p;
            }
        }
    }

    // Process .gnu_debuglink
    auto debuglink = getDebugLink(abfd);
    if (!debuglink.empty())
    {
        std::vector<std::string> search_dirs = {
            config::g_config.symbol_path,
            current_path_
        };
        if (config::g_config.search_symbols)
            search_dirs.push_back("/usr/lib/debug");

        for (auto& dir : search_dirs)
        {
            if (dir.empty()) continue;

            auto found = findFileInDir(dir, debuglink);
            if (!found.empty())
                return found;
        }
    }

    return {};
}

std::pair<std::string, std::string> BFDInfo::splitPath(const std::string& path)
{
    size_t last_slash_pos = path.find_last_of('/');

    if (last_slash_pos == std::string::npos)
    {
        return std::make_pair("", path);
    }
    else
    {
        std::string dir = path.substr(0, last_slash_pos);
        std::string file = path.substr(last_slash_pos + 1);
        return std::make_pair(dir, file);
    }
}

std::string BFDInfo::getBuildId(bfd* abfd)
{
    struct ElfNoteHeader
    {
        uint32_t n_name;
        uint32_t n_desc;
        uint32_t type;
    };
    asection* section = bfd_get_section_by_name(abfd, ".note.gnu.build-id");
    if (section == nullptr)
    {
        return "";
    }

    //bfd_section_size(abfd, section);
    bfd_size_type section_size = section->size;

    //n_name(4bytes)+n_desc(4bytes)+type(4bytes)
    if (section_size <= 12)
    {
        return "";
    }

    std::vector<bfd_byte> buffer(section_size);
    if (!bfd_get_section_contents(abfd, section, buffer.data(), 0, section_size))
    {
        return "";
    }

    ElfNoteHeader header;
    std::memcpy(&header, buffer.data(), sizeof(ElfNoteHeader));

    // Skip header and name
    //----------header(12bytes) -----name(need aligned bt 4bytes)
    int offset = sizeof(ElfNoteHeader) + ((header.n_name + 3) & ~3);

    if (offset + header.n_desc > section_size)
    {
        return "";
    }

    const char hex_chars[] = "0123456789abcdef";

    // Per-byte has 2 hex
    std::string buildid;
    buildid.reserve(header.n_desc * 2 + 1);

    // First byte need '/'
    uint8_t byte = buffer[offset];
    uint8_t low = byte & 0x0F;
    uint8_t high = (byte >> 4) & 0x0F;
    buildid.push_back(hex_chars[high]);
    buildid.push_back(hex_chars[low]);
    buildid.push_back('/');

    // Deal others
    for (int i = 1; i < header.n_desc; i++)
    {
        uint8_t byte = buffer[offset + i];
        uint8_t low = byte & 0x0F;
        uint8_t high = (byte >> 4) & 0x0F;
        buildid.push_back(hex_chars[high]);
        buildid.push_back(hex_chars[low]);
    }
    return buildid;
}

std::string BFDInfo::getDebugLink(bfd* abfd)
{
    asection* section = bfd_get_section_by_name(abfd, ".gnu_debuglink");
    if (section == nullptr)
    {
        return "";
    }
    
    //bfd_section_size(abfd, section);
    bfd_size_type section_size = section->size;
    if (section_size <= 4)
    {
        return "";
    }

    std::vector<bfd_byte> buffer(section_size);
    if (!bfd_get_section_contents(abfd, section, buffer.data(), 0, section_size))
    {
        return "";
    }
    std::string debuglink(reinterpret_cast<char*>(buffer.data()));
    return debuglink;
}

std::string BFDInfo::findFileInDir(const std::string& dir, const std::string& file)
{

    DIR* dp = opendir(dir.c_str());
    if (dp == nullptr)
    {
        return {};
    }
    DEFER(closedir(dp));

    std::string fullPath = dir + "/" + file;
    if (bfd_utils::check_exists_and_is_regular(fullPath.c_str()))
    {
        return fullPath;
    }

    dirent* entry;
    std::string foundPath;
    while ((entry = readdir(dp)) != nullptr)
    {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        fullPath = dir + "/" + entry->d_name;

        // Recursive call for subdirectories
        foundPath = findFileInDir(fullPath, file);
        if (!foundPath.empty())
        {
            // Return immediately if found in a subdirectory
            return foundPath;
        }
    }
    return {};
}

void BFDInfo::loadSymbols(bfd* abfd)
{
    // Start loading symbol
    asymbol** syms_ = nullptr;
    bool dynamic = false;
    unsigned int size = 0;
    long symcnt = bfd_read_minisymbols(abfd, 0, (void**)&syms_, &size);
    if (!symcnt)
    {
        if (syms_ != nullptr)
        {
            free(syms_);
        }
        symcnt = bfd_read_minisymbols(abfd, 1, (void**)&syms_, &size);
        dynamic = true;
    }
    DEFER(free(syms_));

    asymbol* store = bfd_make_empty_symbol(abfd);
    if (!store)
    {
        error_code_ = -1;
        LOG(WARN, "create bfd symbol failed, file: %s", file_.c_str());
        return;
    }

    bfd_byte* p = (bfd_byte*)syms_;
    bfd_byte* pend = p + symcnt * size;
    for (; p < pend; p += size)
    {
        asymbol* sym = bfd_minisymbol_to_symbol(abfd, dynamic, p, store);
        if ((sym->flags & BSF_FUNCTION) == 0)
        {
            continue;
        }
        symbol_info sinfo;
        bfd_get_symbol_info(abfd, sym, &sinfo);
        if (sinfo.type != 'T' && sinfo.type != 't' &&
            sinfo.type != 'W' && sinfo.type != 'w')
        {
            continue;
        }
        if (bfd_utils::startwith(sinfo.name, "__tcf"))
            continue;
        if (bfd_utils::startwith(sinfo.name, "__tz"))
            continue;
        sym_ents_.emplace_back(sinfo.value, sinfo.name);
    }
    // End of loading symbol

    // Sort sym_ents_ for easy search
    std::sort(sym_ents_.begin(), sym_ents_.end(), [](SymbolEnt& l, SymbolEnt& r) {
        return l.addr_ < r.addr_;
        });
}

void BFDCache::loadProcMaps(int pid)
{
    // Load maps
    std::string maps = "/proc/" + std::to_string(pid) + "/maps";
    std::string exe  = "/proc/" + std::to_string(pid) + "/exe";

    std::ifstream file(maps.c_str());
    if (!file.is_open())
    {
        error_code_ = -1;
        LOG(ERROR, "fail to open %s", maps.c_str());
        return;
    }
    DEFER(file.close());

    std::string line;
    while (std::getline(file, line))
    {
        int64_t next_start, next_end, next_inode, next_offset, next_major, next_minor;
        char next_perms[8] = { 0 };
        char next_path[256] = { 0 };

        int n = sscanf(line.c_str(),
            "%lx-%lx %4s %lx %lx:%lx %ld %255s",
            &next_start, &next_end, next_perms,
            &next_offset, &next_major, &next_minor, &next_inode, next_path);

        // Successfully parsed at least up to inode (path is optional)
        if (n < 7)
        { 
            // Skip the line
            LOG(WARN, "Failed to parse line: %s", line);
            continue;
        }

        if (n == 7 || !bfd_utils::check_exist(next_path))
        {
            continue;
        }

        std::string current = next_path;
        auto pt = pt_map_.find(current);
        if (pt != pt_map_.end())
        {
            // Update existing pt
            pt->second->addr_start_ = std::min(pt->second->addr_start_, next_start);
            pt->second->addr_end_   = std::max(pt->second->addr_end_,   next_end);
        }
        else
        {
            // Create new pt
            bool is_exe = bfd_utils::is_same_file(current.c_str(), exe.c_str());
            auto it = pt_map_.emplace(current,
                    std::make_shared<PTLoad>(next_start, next_end, current, is_exe));
            pt_loads_.push_back(it.first->second);
        }
    }

    std::sort(pt_loads_.begin(), pt_loads_.end(), 
        [](const std::shared_ptr<PTLoad>& l, const std::shared_ptr<PTLoad>& r) {
              return l->addr_start_ < r->addr_start_; 
        });

}

void BFDCache::loadSymbolsParallel()
{
    uint32_t total_size = pt_loads_.size();
    uint32_t hardware   = std::thread::hardware_concurrency();

    // If hardware_concurrency is 0 , let thread_num = 2
    hardware = (hardware > 0 ? hardware : 2);

    // Determine the number of threads to launch
    uint32_t thread_num = std::min({total_size, hardware, (uint32_t)config::g_config.bfd_threads });

    // Calculate the number of tasks each thread should handle
    uint32_t quotient  = total_size / thread_num;   // floor division
    uint32_t remainder = total_size % thread_num;   // leftover

    // Prepare a container for worker threads and reserve space
    std::vector<std::thread> workers;
    workers.reserve(thread_num);

    // Launch one thread per chunk, each processing the subrange [start, end)
    for (uint32_t t = 0; t < thread_num; t++)
    {
        // Compute this thread's [start,end) range:
        uint32_t start  = t * quotient + std::min(t, remainder);
        uint32_t count  = quotient + (t < remainder ? 1u : 0u);
        uint32_t end    = start + count;

        // Emplace a new thread to process task
        workers.emplace_back([=]() {
            for (uint32_t i = start; i < end; i++)
            {
                pt_loads_[i]->createBFDInfo();
                pt_loads_[i]->updateLoadAddr();
            }
        });
    }

    // Wait for all task to finish
    for (auto& thread : workers)
    {
        thread.join();
    }

}

void BFDCache::loadSymbols()
{
    // Return early
    if (pt_loads_.size() == 0)
    {
        return;
    }
    LOG(DEBUG, "bfd_threads: %d", config::g_config.bfd_threads);

    // Default case: process all tasks sequentially on the main thread
    if (config::g_config.bfd_threads <= 1)
    {
        for (auto& pt_it : pt_loads_)
        {
            pt_it->createBFDInfo();
            pt_it->updateLoadAddr();
        }
    }
    else
    {
        // Otherwise, dispatch tasks across multiple threads
        loadSymbolsParallel();
    }


}

BFDCache::BFDCache(int pid) : pid_(pid), error_code_(0)
{
    try
    {
        loadProcMaps(pid);

        loadSymbols();
    }
    catch (...)
    {
        error_code_ = -1;
    }
}

Location BFDCache::addr2symbol(int64_t addr)
{
    std::string file = "???";
    std::string func = "???";
    bool is_exe      = false;

    // Find addr in loc_cache
    auto it = loc_cache_.find(addr);
    if (it != loc_cache_.end())
    {
        return it->second;
    }

    auto pt_it = std::lower_bound(pt_loads_.begin(), pt_loads_.end(), addr,
        [](const std::shared_ptr<PTLoad>& pt_load, int64_t addr) {
            if(!pt_load) return false;
            return pt_load->addr_end_ <= addr;
        });

    if (pt_it != pt_loads_.end())
    {
        auto& pt_load = *pt_it;
        if (pt_load != nullptr && pt_load->inRange(addr))
        {
            auto  relative_addr = pt_load->addr2offset(addr);
            auto& sym_ents = pt_load->info_->sym_ents_;
            auto  sym_ent = std::upper_bound(sym_ents.begin(), sym_ents.end(), relative_addr,
                                    [](int64_t addr, SymbolEnt& l) {
                                        return addr < l.addr_;
                                    });

            if (sym_ent == sym_ents.end() || sym_ent == sym_ents.begin())
            {
                LOG(WARN, "symbol not founded, pt_load: %p, relative_addr: %p", pt_load.get(), relative_addr);
            }
            else
            {
                sym_ent--;
                func = bfd_utils::get_demangled_symbol(sym_ent->name_.c_str());
            }

            file   = pt_load->file_;
            is_exe = pt_load->is_exe_;
        }
    }

    it = loc_cache_.emplace(addr, Location(file, func, is_exe)).first;
    return it->second;
}

//std::shared_ptr<PTLoad> BFDCache::find_pt_load(int64_t addr)
//{
//    auto pt_it = std::upper_bound(pt_loads_.begin(), pt_loads_.end(),
//        addr, [](int64_t addr, const std::shared_ptr<PTLoad>& l) {
//            return addr < l->addr_end_; });
//
//    if (pt_it != pt_loads_.end())
//    {
//        return *pt_it;
//    }
//    return nullptr;
//}

