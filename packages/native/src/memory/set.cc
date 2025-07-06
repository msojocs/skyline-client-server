#include "napi.h"
#include "../memory.hh"
#include <cstring>
#include <memory>
#include <map>

namespace SharedMemory {
    static std::map<std::string, std::shared_ptr<SharedMemoryManager>> managerMap;
    Napi::Value set_memory(const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        
        // 参数检查
        if (info.Length() < 2) {
            throw Napi::Error::New(env, "需要两个参数: key和length");
        }
        
        if (!info[0].IsString()) {
            throw Napi::Error::New(env, "第一个参数必须是字符串类型的key");
        }
        
        if (!info[1].IsNumber()) {
            throw Napi::Error::New(env, "第二个参数必须是数字类型的length");
        }
        
        std::string key = info[0].As<Napi::String>().Utf8Value();
        size_t length = info[1].As<Napi::Number>().Uint32Value();
        
        if (length <= 0) {
            throw Napi::Error::New(env, "length必须大于0");
        }
        
        try {
            log("Set memory call.");
            log("Creating SharedMemoryManager...");
            
            // 创建共享内存管理器
            auto manager = managerMap[key] = std::make_shared<SharedMemoryManager>(key, true, length);
            log("SharedMemoryManager created successfully.");
            
            // 获取共享内存的地址和大小
            void* addr = manager->get_address();
            size_t size = manager->get_size();
            
            log("Shared memory created: key=%s, size=%zu, address=%p", 
                key.c_str(), size, addr);
            
            // 获取数据区域的地址
            void* data_addr = static_cast<char*>(addr) + sizeof(SharedMemoryHeader);
            memset(data_addr, 0, length);
            // 初始分配时，存储key。
            auto str = "key:" + key;
            memcpy(data_addr, str.c_str(), str.length());
            
            // 创建ArrayBuffer，直接映射到共享内存
            auto buffer = Napi::ArrayBuffer::New(env, data_addr, length);
            
            return buffer;
            
        } catch (const std::exception& e) {
            log("Error: %s", e.what());
            throw Napi::Error::New(env, e.what());
        } catch (...) {
            log("Unknown error occurred");
            throw Napi::Error::New(env, "设置共享内存时发生未知错误");
        }
    }
}