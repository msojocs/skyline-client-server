#include "napi.h"
#include "../memory.hh"
#include <cstring>
#include <memory>
#include <map>

namespace SharedMemory {
    // 全局变量来保存共享内存资源
    static std::map<std::string, std::shared_ptr<SharedMemoryManager>> managerMap;
    Napi::Value get_memory(const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        
        // 参数检查
        if (info.Length() < 1) {
            throw Napi::Error::New(env, "需要一个参数: key");
        }
        
        if (!info[0].IsString()) {
            throw Napi::Error::New(env, "参数必须是字符串类型的key");
        }
        
        std::string key = info[0].As<Napi::String>().Utf8Value();
        
        try {
            log("Get memory call.");
            log("Creating SharedMemoryManager...");
            
            // 创建共享内存管理器
            auto manager = managerMap[key] = std::make_shared<SharedMemoryManager>(key, false);
            log("SharedMemoryManager created successfully.");
            
            // 获取共享内存的地址和大小
            void* addr = manager->get_address();
            size_t size = manager->get_size();
            
            log("Shared memory opened: key=%s, size=%zu, address=%p", 
                key.c_str(), size, addr);
            
            // 读取头部信息
            SharedMemoryHeader* header = static_cast<SharedMemoryHeader*>(addr);
            size_t data_size = header->size;
            
            log("Header information: size=%zu", data_size);
            
            // 获取数据区域的地址
            void* data_addr = static_cast<char*>(addr) + sizeof(SharedMemoryHeader);
            
            auto deleter = [](void* /*data*/, void* /*hint*/) {
                log("Buffer cleanup callback called.");
                // 这里不需要做任何事情，因为共享内存和互斥锁会在程序退出时自动清理
            };

            // 创建ArrayBuffer，直接映射到共享内存
            auto buffer = Napi::ArrayBuffer::New(env, data_addr, data_size, deleter);
            
            
            return buffer;
            
        } catch (const std::exception& e) {
            log("Error: %s", e.what());
            throw Napi::Error::New(env, e.what());
        } catch (...) {
            log("Unknown error occurred");
            throw Napi::Error::New(env, "获取共享内存时发生未知错误");
        }
    }
}