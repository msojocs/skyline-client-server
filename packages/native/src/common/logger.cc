#include <napi.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Logger {
    std::shared_ptr<spdlog::logger> logger;
    #if defined (_SKYLINE_CLIENT_)
    #ifdef _WIN32
    #define LOG_FILE "D:/Log/Skyline/client_log.log"
    #else
    #define LOG_FILE "/tmp/Log/Skyline/client_log.log"
    #endif
    #else
    #ifdef _WIN32
    #define LOG_FILE "D:/Log/Skyline/server_log.log"
    #else
    #define LOG_FILE "/tmp/Log/Skyline/server_log.log"
    #endif
    #endif

    // 设置控制台回调函数
    void Init() {
        std::vector<spdlog::sink_ptr> sinks;
        // auto stdout_log = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        // sinks.push_back(stdout_log);
        auto file_log = std::make_shared<spdlog::sinks::daily_file_sink_mt>(LOG_FILE, 0, 0);
        sinks.push_back(file_log);
        
        logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e.%f] [%l] %v");
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::err);
        spdlog::register_logger(logger);
        logger->info("Logger initialized successfully.");
    }
}