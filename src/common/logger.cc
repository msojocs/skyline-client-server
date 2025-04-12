#include <napi.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

namespace Logger {
    std::shared_ptr<spdlog::logger> logger;
    #if defined (_SKYLINE_CLIENT_)
    #define LOG_FILE "D:/Log/SkylineClient/daily_log.log"
    #else
    #define LOG_FILE "D:/Log/SkylineServer/daily_log.log"
    #endif

    // 设置控制台回调函数
    void Init() {
        logger = spdlog::daily_logger_mt("daily_logger", LOG_FILE, 0, 0);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);
    }
}