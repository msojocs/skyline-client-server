#ifndef SHARED_MEMORY_MANAGER_HH
#define SHARED_MEMORY_MANAGER_HH

#include <memory>
#include <string>

// 平台特定的头文件
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#endif
namespace SharedMemory {
    // 共享内存头部结构
    struct SharedMemoryHeader {
        size_t size;          // 用户数据大小
        int version;          // 版本号
        int data_start_offset = 0; // 数据起始偏移
        int data_end_offset = 0;   // 数据结束偏移
    };
    // 共享内存管理器类
    class SharedMemoryManager : public std::enable_shared_from_this<SharedMemoryManager> {
    public:
        // 构造函数
        SharedMemoryManager(const std::string& key, bool create = false, size_t size = 0);
        
        // 析构函数
        ~SharedMemoryManager();
        
        // 获取共享内存地址
        void* get_address() const { return address_; }
        
        // 获取共享内存数据区大小，不含头部
        size_t get_size() const { return size_; }
        
        // 获取文件路径
        const std::string& get_file_path() const { return file_path_; }
        
        // 获取版本号
        int get_version() const { 
            if (address_) {
                return static_cast<SharedMemoryHeader*>(address_)->version;
            }
            return 0;
        }
        
    private:
        std::string key_;           // 共享内存键名
        size_t size_;               // 数据区大小
        void* address_;             // 共享内存地址
        std::string file_path_;     // 文件路径

#ifdef _WIN32
        HANDLE file_mapping_;       // 文件映射句柄
        HANDLE mutex_;              // 互斥锁句柄
        
        // 创建文件映射
        bool create_mapping(HANDLE file_handle, size_t mapping_size);
#else
        sem_t* mutex_;              // 互斥锁
#endif
    };
}
#endif // SHARED_MEMORY_MANAGER_HH