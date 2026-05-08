#ifndef BFD_CACHE_H
#define BFD_CACHE_H

#include <string>
#include <vector>
#include <map>
#include <memory>


#include "bfd/config.h"
#include "utils/location.h"
#include <bfd.h>

// elf need these headers
#include <gelf.h>
#include <libelf.h>


class SymbolEnt
{
public:
    SymbolEnt(int64_t addr, std::string name) : addr_(addr), name_(name) {}

    int64_t     addr_;
    std::string name_;
};

class BFDInfo
{
public:
    BFDInfo(std::string file, bool is_exe);

private:
    using BfdPtr = std::unique_ptr<bfd, decltype(&bfd_close)>;

    BfdPtr openBfd(const std::string& path);
    std::string findFileWithSymbols(bfd* abfd, const std::string& orig, bool is_exe);
    std::string getBuildId(bfd* abfd);
    std::string getDebugLink(bfd* abfd);
    std::string findFileInDir(const std::string& dir, const std::string& file);
    std::pair<std::string, std::string> splitPath(const std::string& path);


    void loadSymbols(bfd* abfd);

public:
    int error_code_;
    std::string             file_;          // file in maps
    std::string             debug_file_;    // debuglink

    std::string             load_file_;      // actual loading
    std::vector<SymbolEnt>  sym_ents_;      // symbol table
};

class PTLoad
{
public:
    PTLoad(int64_t start, int64_t end, std::string file, bool is_exe = false);

    int64_t addr2offset(const int64_t addr) const
    {
        return addr - (addr_start_ - load_vaddr_);
    }

    bool inRange(const int64_t addr) const
    {
        return (addr >= addr_start_) && (addr < addr_end_);
    }

    void updateLoadAddr();
    void createBFDInfo();

    int64_t addr_start_;
    int64_t addr_end_;
    int64_t load_vaddr_;
    bool is_exe_;
    std::string file_;
    std::shared_ptr<BFDInfo> info_;
};

class BFDCache
{
public:
    BFDCache() = default;
    ~BFDCache() = default;

    BFDCache(int pid);

    int getErrorCode() const { return error_code_; }
    void loadProcMaps(int pid);
    void loadSymbols();
    void loadSymbolsParallel();

    Location addr2symbol(int64_t addr);
    //std::shared_ptr<PTLoad> find_pt_load(int64_t addr);

private:
    int pid_;
    int error_code_;
    std::vector<std::shared_ptr<PTLoad>>            pt_loads_;
    std::map<std::string, std::shared_ptr<PTLoad>>  pt_map_;
    std::map<int64_t, Location>                     loc_cache_;
};

#endif // !BFD_CACHE_H
