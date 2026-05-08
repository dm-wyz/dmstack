#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <semaphore.h>          // semaphore
#include <sys/stat.h>           // stat
#include <sys/mman.h>           // mmap
#include <unistd.h>             // fork getpid getppid
#include <fcntl.h>              // open O_CREAT
#include <iostream>
#include <string>
#include <cerrno>

#include "log/log.h"

class SharedMemory
{
public:
    SharedMemory(const std::string& shm_name, bool destroy_on_exit = false)
        : shm_name_(shm_name), destroy_(destroy_on_exit),
        fd_(-1), data_(nullptr), size_(0), error_code_(0) {
    }

    ~SharedMemory()
    {
        if (destroy_)
        {
            destroy();
            return;
        }

        if (data_)
        {
            munmap(data_, size_);
        }
        if (fd_ != -1)
        {
            close(fd_);
        }
    }

    void destroy()
    {
        if (data_)
        {
            munmap(data_, size_);
            data_ = nullptr;
        }

        if (fd_ != -1)
        {
            close(fd_);
            fd_ = -1;
        }

        if (!shm_name_.empty())
        {
            if (shm_unlink(shm_name_.c_str()) == -1)
            {
                error_code_ = errno;
                LOG(ERROR, "shm_unlink: %s", strerror(error_code_));
            }
        }
        size_ = 0;
    }


    void* allocate(size_t size)
    {
        if (size == 0)
        {
            return nullptr;
        }

        if (data_)
        {
            LOG(ERROR, "SharedMemory has been allocated");
            error_code_ = -1;
            return nullptr;
        }

        if (shm_name_.empty())
        {
            data_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
            if (data_ == MAP_FAILED)
            {
                error_code_ = errno;
                LOG(ERROR, "mmap:%s", strerror(error_code_));
                return nullptr;
            }

            size_ = size;
            return data_;
        }

        fd_ = shm_open(shm_name_.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);
        if (fd_ == -1)
        {
            error_code_ = errno;
            LOG(ERROR, "shm_open:%s", strerror(error_code_));
            return nullptr;
        }

        if (ftruncate(fd_, size) == -1)
        {
            error_code_ = errno;
            LOG(ERROR, "ftruncate:%s", strerror(error_code_));
            return nullptr;
        }

        data_ = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (data_ == MAP_FAILED)
        {
            error_code_ = errno;
            LOG(ERROR, "mmap:%s", strerror(error_code_));
            return nullptr;
        }

        size_ = size;
        return data_;
    }

    void* open()
    {
        if (data_)
        {
            // has been opened
            return data_;
        }

        fd_ = shm_open(shm_name_.c_str(), O_RDWR, 0666);
        if (fd_ == -1)
        {
            error_code_ = errno;
            LOG(ERROR, "shm_open:%s", strerror(error_code_));
            return nullptr;
        }

        struct stat shm_stat;
        if (fstat(fd_, &shm_stat) == -1)
        {
            error_code_ = errno;
            LOG(ERROR, "fstat:%s", strerror(error_code_));
            return nullptr;
        }
        size_ = shm_stat.st_size;
        if (size_ == 0)
        {
            return nullptr;
        }

        data_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (data_ == MAP_FAILED)
        {
            data_ = nullptr;
            error_code_ = errno;
            LOG(ERROR, "mmap:%s", strerror(error_code_));
            return nullptr;
        }

        return data_;
    }

    void* data() const { return data_; }
    int size() const { return size_; }

    int getErrorCode() const { return error_code_; }
    void setDestroyFlag(bool destroy) { destroy_ = destroy; }

private:
    void* data_;
    int size_;

    int fd_;
    int error_code_;
    bool destroy_;
    std::string shm_name_;

    SharedMemory(const SharedMemory&) = delete;
    SharedMemory& operator=(const SharedMemory&) = delete;
};

class Semaphore
{
public:
    Semaphore(int pshared = 1, unsigned int value = 0) : error_code_(0)
    {
        sem_ = (sem_t*)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        if (sem_ == MAP_FAILED)
        {
            error_code_ = -1;
            return;
        }

        if (sem_init(sem_, pshared, value) == -1)
        {
            munmap(sem_, sizeof(sem_t));
            sem_ = nullptr;

            error_code_ = -1;
            return;
        }
    }

    ~Semaphore()
    {
        if (sem_ != nullptr && sem_ != MAP_FAILED)
        {
            sem_destroy(sem_);
            munmap(sem_, sizeof(sem_t));
        }
    }

    Semaphore(Semaphore&&) = default;
    Semaphore& operator=(Semaphore&&) = default;

    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;



    void wait() { sem_wait(sem_); }
    void post() { sem_post(sem_); }
    sem_t* get() { return sem_; }

private:
    sem_t* sem_;
    int error_code_;
};

#endif // !SHARED_MEMORY_H
