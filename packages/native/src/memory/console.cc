#include "../memory.hh"
#include <cstdarg>
#include <cstdio>
#include <spdlog/spdlog.h>

namespace SharedMemory {
    // 全局回调函数
    Napi::FunctionReference console_callback;

    // 设置控制台回调函数
    Napi::Value set_console(const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        
        if (info.Length() != 1) {
            throw Napi::Error::New(env, "参数长度必须为1!");
        }
        if (!info[0].IsFunction()) {
            throw Napi::Error::New(env, "参数必须是函数!");
        }

        // 保存回调函数
        console_callback = Napi::Persistent(info[0].As<Napi::Function>());
        
        return env.Undefined();
    }

    // 清理控制台回调函数
    void cleanup_console() {
        if (!console_callback.IsEmpty()) {
            console_callback.Reset();
        }
    }

    // 日志辅助函数 - 字符串版本
    void log(const std::string& message) {
        if (!console_callback.IsEmpty()) {
            try {
                Napi::Env env = console_callback.Env();
                Napi::HandleScope scope(env);
                
                // 使用 try-catch 捕获可能的异常
                try {
                    console_callback.Call({Napi::String::New(env, message)});
                } catch (const Napi::Error& error) {
                    spdlog::error("回调函数执行失败: {}", error.Message());
                }
            } catch (const std::exception& e) {
                spdlog::error("回调函数执行异常: {}", e.what());
            } catch (...) {
                spdlog::error("回调函数执行未知异常");
            }
        }
    }

    // 日志辅助函数 - 格式化版本
    void log(const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        
        if (!console_callback.IsEmpty()) {
            try {
                vsnprintf(buffer, sizeof(buffer), format, args);
                va_end(args);

                Napi::Env env = console_callback.Env();
                Napi::HandleScope scope(env);
                
                // 使用 try-catch 捕获可能的异常
                try {
                    console_callback.Call({Napi::String::New(env, buffer)});
                } catch (const Napi::Error& error) {
                    spdlog::error("回调函数执行失败: {}", error.Message());
                }
            } catch (const std::exception& e) {
                spdlog::error("回调函数执行异常: {}", e.what());
            } catch (...) {
                spdlog::error("回调函数执行未知异常");
            }
        }
        else {
            char buffer[1024];
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            spdlog::info("{}", buffer);
        }
    }
} 