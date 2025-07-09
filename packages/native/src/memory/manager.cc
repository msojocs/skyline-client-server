#include "manager.hh"
#include <cstdio>
#include <cstring>
#include <string.h>
#include <sys/stat.h>
#include <stdexcept>

#ifdef _WIN32
#include <direct.h> // 用于Windows目录创建
#include <shlobj.h> // 用于获取用户目录
#else
#include <errno.h>    // 用于错误处理
#include <sys/mman.h>
#endif
#include "../common/logger.hh"
using Logger::logger;
namespace SharedMemory {


    // 检测是否在Wine环境下运行
    bool is_running_under_wine() {
#ifdef _WIN32
        HMODULE hntdll = GetModuleHandleA("ntdll.dll");
        if (hntdll) {
            typedef const char* (*wine_get_version)();
            wine_get_version wine_get_version_func = (wine_get_version)GetProcAddress(hntdll, "wine_get_version");
            if (wine_get_version_func) {
                return true;
            }
        }
#endif
        return false;
    }

    // 创建目录的跨平台函数
    bool create_directory(const std::string& path) {
#ifdef _WIN32
        return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
        return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
    }

    SharedMemoryManager::SharedMemoryManager(const std::string& key, bool create, size_t size) 
        : key_(key), size_(size), address_(nullptr)
#ifdef _WIN32
        , file_mapping_(nullptr), mutex_(nullptr)
#else
        , mutex_(nullptr)
#endif
    {
        // 计算实际需要分配的大小（包括头部）
        size_t total_size = sizeof(SharedMemoryHeader) + size;
        
#ifdef _WIN32
        // Windows实现
        // 创建互斥锁名称
        std::string mutex_name = "Global\\SharedMemoryMutex_" + key;
        
        // 检查是否在Wine环境下运行
        bool is_wine = is_running_under_wine();
        
        std::string file_path;
        
        if (is_wine) {
            // Wine环境下使用/dev/shm目录
            file_path = "/dev/shm/skyline_" + key + ".dat";
            
            // 确保/dev/shm目录存在
            // 在Wine环境下，这个目录应该已经存在，但为了安全起见，我们检查一下
            struct stat st;
            if (stat("/dev/shm", &st) != 0) {
                // log("Warning: /dev/shm directory does not exist in Wine environment");
            }
        } else {
            // 原生Windows环境下使用用户目录
            // 获取用户目录
            char user_path[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, user_path))) {
                // log("User path: %s", user_path);
            } else {
                // 如果获取用户目录失败，使用当前目录
                GetCurrentDirectoryA(MAX_PATH, user_path);
                // log("Using current directory: %s", user_path);
            }
            
            // 创建文件路径
            std::string shared_memory_dir = std::string(user_path) + "\\SharedMemory";
            file_path = shared_memory_dir + "\\skyline_" + key + ".dat";
            
            // 确保目录存在
            create_directory(shared_memory_dir);
        }
        logger->debug("CreateMutexA");
        // 创建或打开互斥锁
        mutex_ = CreateMutexA(NULL, FALSE, mutex_name.c_str());
        if (!mutex_) {
            DWORD error = GetLastError();
            logger->error("Failed to create mutex, error code: %lu", error);
            throw std::runtime_error("Failed to create mutex");
        }
        
        // 获取互斥锁
        if (WaitForSingleObject(mutex_, 5000) != WAIT_OBJECT_0) {
            DWORD error = GetLastError();
            // log("Failed to acquire mutex, error code: %lu", error);
            CloseHandle(mutex_);
            mutex_ = nullptr;
            throw std::runtime_error("Failed to acquire mutex");
        }
        
        try {
            // 创建或打开文件
            HANDLE file_handle = INVALID_HANDLE_VALUE;
            if (create) {
                // 创建新文件
                file_handle = CreateFileA(
                    file_path.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,  // 允许其他进程读写
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
                
                if (file_handle == INVALID_HANDLE_VALUE) {
                    DWORD error = GetLastError();
                    logger->error("Failed to create file, error code: {}", error);
                    ReleaseMutex(mutex_);
                    CloseHandle(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to create file");
                }
                
                // 设置文件大小
                LARGE_INTEGER file_size;
                file_size.QuadPart = total_size;
                if (!SetFilePointerEx(file_handle, file_size, NULL, FILE_BEGIN) || 
                    !SetEndOfFile(file_handle)) {
                    DWORD error = GetLastError();
                    // log("Failed to set file size, error code: %lu", error);
                    CloseHandle(file_handle);
                    ReleaseMutex(mutex_);
                    CloseHandle(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to set file size");
                }
            } else {
                // 打开现有文件
                file_handle = CreateFileA(
                    file_path.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,  // 允许其他进程读写
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
                
                if (file_handle == INVALID_HANDLE_VALUE) {
                    DWORD error = GetLastError();
                    // log("Failed to open file, error code: %lu", error);
                    ReleaseMutex(mutex_);
                    CloseHandle(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to open file");
                }
            }
            
            // 创建文件映射
            const size_t MAX_MAPPING_SIZE = 1024 * 1024 * 1024; // 1GB per mapping
            size_t remaining_size = total_size;
            size_t offset = 0;
            
            // 创建映射
            bool success = create_mapping(file_handle, remaining_size);
            if (!success) {
                CloseHandle(file_handle);
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
                mutex_ = nullptr;
                throw std::runtime_error("Failed to create mapping");
            }
            
            // 如果是新创建的共享内存，初始化头部
            if (create) {
                SharedMemoryHeader* header = static_cast<SharedMemoryHeader*>(address_);
                header->size = size;
                header->version = 1;
                // log("Initialized shared memory header: size=%zu, version=%d", size, header->version);
            }
            else {
                // 读取头部信息
                SharedMemoryHeader* header = static_cast<SharedMemoryHeader*>(address_);
                size = header->size;
                // log("Read shared memory header: size=%zu, version=%d", size, header->version);
                
                // 以头部信息为基准，重新映射
                success = create_mapping(file_handle, size + sizeof(SharedMemoryHeader));
                if (!success) {
                    CloseHandle(file_handle);
                    ReleaseMutex(mutex_);
                    CloseHandle(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to create mapping with header size");
                }
            }
            
            // 关闭文件句柄，文件映射会保持文件打开
            CloseHandle(file_handle);
            
            // 存储文件路径
            file_path_ = file_path;
            
            // log("Shared memory %s: key=%s, size=%zu, address=%p, file=%s", 
            //     create ? "created" : "opened", 
            //     key.c_str(), 
            //     size, 
            //     address_,
            //     file_path_.c_str());
                
        } catch (...) {
            // 确保在发生异常时释放资源
            if (address_) {
                UnmapViewOfFile(address_);
                address_ = nullptr;
            }
            
            if (file_mapping_) {
                CloseHandle(file_mapping_);
                file_mapping_ = nullptr;
            }
            
            if (mutex_) {
                ReleaseMutex(mutex_);
                CloseHandle(mutex_);
                mutex_ = nullptr;
            }
            
            throw;
        }
        
        // 释放互斥锁
        ReleaseMutex(mutex_);
#else
        // Linux实现
        // 创建共享内存名称
        std::string shm_name = "/skyline_" + key + ".dat";
        
        // 创建互斥锁名称
        std::string mutex_name = "/skyline_mutex_" + key;
        
        // 创建或打开互斥锁
        if (create) {
            mutex_ = sem_open(mutex_name.c_str(), O_CREAT, 0644, 1);
        }
        else {
            mutex_ = sem_open(mutex_name.c_str(), O_RDWR);
        }
        if (mutex_ == SEM_FAILED) {
            logger->error("Failed to create mutex, error: {}", strerror(errno));
            throw std::runtime_error("Failed to create mutex");
        }
        
        // 获取互斥锁
        if (sem_wait(mutex_) != 0) {
            logger->error("Failed to acquire mutex, error: {}", strerror(errno));
            sem_close(mutex_);
            mutex_ = nullptr;
            throw std::runtime_error("Failed to acquire mutex");
        }
        
        try {
            int flags = O_RDWR;
            if (create) {
                flags |= O_CREAT;
            }
            
            // 创建或打开共享内存
            int fd = shm_open(shm_name.c_str(), flags, 0644);
            if (fd == -1) {
                logger->error("Failed to open shared memory, error: {}", strerror(errno));
                sem_post(mutex_);
                sem_close(mutex_);
                mutex_ = nullptr;
                throw std::runtime_error("Failed to open shared memory");
            }
            
            if (create) {
                // 设置共享内存大小
                if (ftruncate(fd, total_size) == -1) {
                    logger->error("Failed to set shared memory size, error: {}", strerror(errno));
                    close(fd);
                    sem_post(mutex_);
                    sem_close(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to set shared memory size");
                }
            }
            else {
                
                // 映射共享内存
                address_ = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (address_ == MAP_FAILED) {
                    logger->error("Failed to map shared memory, error: {}", strerror(errno));
                    sem_post(mutex_);
                    sem_close(mutex_);
                    mutex_ = nullptr;
                    throw std::runtime_error("Failed to map shared memory");
                }
                // 读取头部信息
                SharedMemoryHeader* header = static_cast<SharedMemoryHeader*>(address_);
                size = header->size;
                total_size = size + sizeof(SharedMemoryHeader);
                size_ = size;
                logger->error("Read shared memory header: size={}, version={}", size, header->version);
                munmap(address_, total_size);
            }
            
            // 映射共享内存
            address_ = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            close(fd);
            
            if (address_ == MAP_FAILED) {
                logger->error("Failed to map shared memory, error: {}", strerror(errno));
                sem_post(mutex_);
                sem_close(mutex_);
                mutex_ = nullptr;
                throw std::runtime_error("Failed to map shared memory");
            }
            
            // 如果是新创建的共享内存，初始化头部
            if (create) {
                SharedMemoryHeader* header = static_cast<SharedMemoryHeader*>(address_);
                header->size = size;
                header->version = 1;
                logger->info("Initialized shared memory header: size={}, version={}", size, header->version);
            }
            
            // 存储共享内存名称
            file_path_ = shm_name;
            
            logger->info("Shared memory {}: key={}, size={}, address={}", 
                create ? "created" : "opened", 
                key.c_str(), 
                size, 
                address_);
                
        } catch (...) {
            // 确保在发生异常时释放资源
            if (address_ && address_ != MAP_FAILED) {
                munmap(address_, total_size);
                address_ = nullptr;
            }
            
            if (mutex_) {
                sem_post(mutex_);
                sem_close(mutex_);
                mutex_ = nullptr;
            }
            
            throw;
        }
        
        // 释放互斥锁
        sem_post(mutex_);
#endif
    }
    
    SharedMemoryManager::~SharedMemoryManager() {
        printf("Destroying shared memory manager: key=%s, file=%s\n", 
            key_.c_str(), 
            file_path_.c_str());
#ifdef _WIN32
        // Windows实现
        // 释放资源
        if (address_) {
            UnmapViewOfFile(address_);
            address_ = nullptr;
        }
        
        if (file_mapping_) {
            CloseHandle(file_mapping_);
            file_mapping_ = nullptr;
        }
        
        if (mutex_) {
            ReleaseMutex(mutex_);
            CloseHandle(mutex_);
            mutex_ = nullptr;
        }
#else
        // Linux实现
        // 释放资源
        if (address_ && address_ != MAP_FAILED) {
            size_t total_size = sizeof(SharedMemoryHeader) + size_;
            munmap(address_, total_size);
            address_ = nullptr;
        }
        
        if (mutex_) {
            sem_post(mutex_);
            sem_close(mutex_);
            mutex_ = nullptr;
        }
#endif
        
        printf("Shared memory manager destroyed: key=%s, file=%s\n", 
            key_.c_str(), 
            file_path_.c_str());
    }

    #ifdef _WIN32
    bool SharedMemoryManager::create_mapping(HANDLE file_handle, size_t mapping_size) {
        // 如果已存在映射，先清理
        if (file_mapping_) {
            CloseHandle(file_mapping_);
            file_mapping_ = nullptr;
        }
        if (address_) {
            UnmapViewOfFile(address_);
            address_ = nullptr;
        }
        
        // 创建文件映射
        file_mapping_ = CreateFileMappingA(
            file_handle,          // 使用实际文件
            NULL,                 // 默认安全属性
            PAGE_READWRITE,       // 读写权限
            0,                    // 最大大小的高32位
            mapping_size,         // 最大大小的低32位
            NULL                  // 不使用命名映射
        );
        
        if (!file_mapping_) {
            DWORD error = GetLastError();
            // log("Failed to create file mapping, error code: %lu, size: %zu", error, mapping_size);
            return false;
        }
        
        // 映射视图
        address_ = MapViewOfFile(
            file_mapping_,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            mapping_size
        );
        
        if (!address_) {
            DWORD error = GetLastError();
            // log("Failed to map view of file, error code: %lu, size: %zu", error, mapping_size);
            CloseHandle(file_mapping_);
            file_mapping_ = nullptr;
            return false;
        }
        
        return true;
    }
    #endif

} 