#ifndef BACKTRACE_H
#define BACKTRACE_H


#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unordered_map>

#include "log/log.h"

class BackTrace;
class ThreadInfo
{
public:
    ThreadInfo(int tid, const std::string& name = "", BackTrace* bt = nullptr) :
        tid_(tid), name_(name), bt_(nullptr) {}

    int tid() const
    { 
        return tid_;
    }
    
    std::string name() const
    {
        return name_;
    }

    BackTrace* bt() const
    {
        return bt_;
    }

#ifndef _WIN32
    static std::unordered_map<int, std::string> getThreads(int pid, bool thread_only);
    bool isPidStopped();
#endif

private:
    int                     tid_;
    std::string             name_;
    BackTrace*              bt_;
};

class BackTrace
{
public:
    BackTrace() = default;
    BackTrace(int tid) :tid_(tid)
    {
        addr_.reserve(256);
    }

    void addAddr(int64_t addr)
    {
        addr_.push_back(addr); 
    }

    const std::vector<int64_t>& addrs() const
    {
        return addr_;
    }

    int tid() const
    {
        return tid_;
    }

    void setName(std::string name)
    {
        name_ = name;
    }
    void setTid(int tid)
    {
        tid_ = tid;
    }

    bool operator<(const BackTrace& other) const
    {
        const auto as = addr_.size();
        const auto bs = other.addrs().size();
        if (as < bs) return true;
        if (as > bs) return false;
        return addr_ < other.addrs();
    }

    size_t serializedSize() const
    {
        return sizeof(tid_) + sizeof(unsigned int) + addr_.size() * sizeof(int64_t);
    }

    size_t serializeToBuffer(char* buffer) const
    {
        char* ptr = buffer;
        int offset = 0;
        unsigned int addr_count = static_cast<unsigned int>(addr_.size());

        // first 4 byte is tid
        std::memcpy(ptr + offset, &tid_, sizeof(tid_));
        offset += sizeof(tid_);

        // second 4 byte is addr_.size()
        std::memcpy(ptr + offset, &addr_count, sizeof(addr_count));
        offset += sizeof(addr_count);

        std::memcpy(ptr + offset, addr_.data(), addr_count * sizeof(int64_t));
        offset += addr_count * sizeof(int64_t);

        return offset;
    }

    void deserializeFromBuffer(const char* buffer)
    {
        const char* ptr = buffer;
        int offset = 0;
        unsigned int addr_count;

        std::memcpy(&tid_, ptr + offset, sizeof(tid_));
        offset += sizeof(tid_);
        
        std::memcpy(&addr_count, ptr + offset, sizeof(addr_count));
        offset += sizeof(addr_count);
        
        addr_.resize(addr_count);
        std::memcpy(addr_.data(), ptr + offset, addr_count * sizeof(int64_t));
    }

public:
    int                     tid_;
    std::string             name_;
    std::vector<int64_t>    addr_;
};

class BackTraces
{
public:
    BackTraces() = default;
    ~BackTraces() = default;

#ifndef _WIN32
    BackTraces(std::unordered_map<int, std::string>& task);
#endif

    const std::vector<BackTrace>& getBackTraces() const
    { 
        return bts_;
    }
    int getErrorCode() const
    {
        return error_code_;
    }

    size_t serializedSize() const
    {
        size_t size = sizeof(unsigned int); // 用于存储 bts_ 的大小
        for (const auto& bt : bts_)
        {
            size += bt.serializedSize();
        }
        return size;
    }

    size_t serializeToBuffer(char* buffer, size_t buffer_size) const
    {
        //assert(buffer_size >= serializedSize());

        char* ptr = buffer;
        int offset = 0;
        unsigned int count = static_cast<unsigned int>(bts_.size());

        // first 4 byte is bts_.size()
        std::memcpy(ptr + offset, &count, sizeof(count));
        offset += sizeof(count);

        for (const auto& bt : bts_)
        {
            offset += bt.serializeToBuffer(ptr + offset);
        }
        LOG(DEBUG, "serialize buffer size: %d", offset);

        return offset;
    }

    void deserializeFromBuffer(const char* buffer, size_t buffer_size)
    {
        const char* ptr = buffer;
        int offset = 0;
        unsigned int count;

        // first 4 byte is bts_.size()
        std::memcpy(&count, ptr + offset, sizeof(count));
        offset += sizeof(count);

        bts_.resize(count);
        for (auto& bt : bts_)
        {
            bt.deserializeFromBuffer(ptr + offset);
            offset += bt.serializedSize();
        }

        LOG(DEBUG, "deserialize buffer size: %d", offset);
    }

private:
    std::vector<BackTrace>  bts_;
    int                     error_code_;
};
#endif
