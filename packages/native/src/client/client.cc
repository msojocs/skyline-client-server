#include <spdlog/spdlog.h>
#include <napi.h>
#include <stdexcept>
#include <sys/types.h>
#include "skyline_shell.hh"
#include "socket_client.hh"
#include "skyline_debug_info.hh"
#include "../common/logger.hh"
#include "../common/convert.hh"
#include "crash_handler.hh"
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <sstream>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

using Logger::logger;


static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  try {
    // 初始化崩溃处理器
    #ifdef _WIN32
    CrashHandler::init();
    #endif
    
    // 初始化日志
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting Skyline Client...");
    Logger::Init();
    
    SocketClient::initSocket(env);
    // 发送HTTP请求检查重启状态
    auto log = env.Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();
    log.Call({Napi::String::New(env, "正在检查Skyline服务器状态...")});
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::asio::ip::tcp::socket socket(io_context);
    
    // 设置连接和操作超时为10秒
    boost::asio::steady_timer connect_timer(io_context);
    connect_timer.expires_after(std::chrono::seconds(10));
    
    boost::system::error_code ec;
    auto endpoints = resolver.resolve("127.0.0.1", "8086", ec);
    if (ec) {
      #ifdef _WIN32
      wchar_t wbuf[1024] = {0};
      FormatMessageW(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          ec.value(),
          MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
          wbuf,
          1024,
          NULL
      );
      
      // 将宽字符转换为UTF-8
      int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
      std::string utf8_str(utf8_size, 0);
      WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, &utf8_str[0], utf8_size, NULL, NULL);
      
      throw std::runtime_error("地址解析失败: " + utf8_str);
      #else
      throw std::runtime_error("Address resolution failed: " + ec.message());
      #endif
    }
    // 使用带超时的连接
    boost::asio::connect(socket, endpoints, ec);
    if (ec) {
      #ifdef _WIN32
      wchar_t wbuf[1024] = {0};
      FormatMessageW(
          FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          ec.value(),
          MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED),
          wbuf,
          1024,
          NULL
      );
      
      // 将宽字符转换为UTF-8
      int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
      std::string utf8_str(utf8_size, 0);
      WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, &utf8_str[0], utf8_size, NULL, NULL);
      
      throw std::runtime_error("连接失败: " + utf8_str);
      #else
      throw std::runtime_error("Connection failed: " + ec.message());
      #endif
    }
    
    // 设置读写操作的超时
    #ifdef _WIN32
    socket.set_option(boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_RCVTIMEO>(10000));
    socket.set_option(boost::asio::detail::socket_option::integer<SOL_SOCKET, SO_SNDTIMEO>(10000));
    #else
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 seconds
    timeout.tv_usec = 0;
    socket.native_handle();  // Get the native socket handle
    if (setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Failed to set receive timeout");
    }
    if (setsockopt(socket.native_handle(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("Failed to set send timeout");
    }
    #endif

    std::string request = "GET /restart HTTP/1.1\r\n"
                        "Host: 127.0.0.1:8086\r\n"
                        "Connection: close\r\n\r\n";
  
    boost::asio::write(socket, boost::asio::buffer(request));
    
    boost::asio::streambuf response;
    boost::system::error_code error;
    
    // 读取HTTP头和响应体
    std::stringstream ss;
    while (boost::asio::read(socket, response, boost::asio::transfer_at_least(1), error)) {
      ss << &response;
    }
    
    if (error != boost::asio::error::eof) {
      throw std::runtime_error("响应读取失败: " + error.message());
    }

    std::string response_str = ss.str();
    logger->debug("Full HTTP response: {}", response_str);
    
    std::size_t body_pos = response_str.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
      std::string raw_body = response_str.substr(body_pos + 4);
      logger->debug("Raw response body: {}", raw_body);
      
      // 处理 chunked 编码：跳过第一行的chunk大小（2），直接读取内容行
      std::istringstream body_stream(raw_body);
      std::string chunk_size_line;
      std::getline(body_stream, chunk_size_line);
      
      std::string content;
      std::getline(body_stream, content);
      
      // 清理内容中的空白字符
      content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());
      content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());
      
      logger->debug("Extracted content: '{}'", content);
      
      if (content == "ok") {
        logger->info("重启状态检查: ok");
        log.Call({Napi::String::New(env, "Skyline服务器重启状态: ok")});
      } else {
        logger->warn("重启状态检查失败. 响应: '{}'", content);
        throw std::runtime_error("Skyline服务器重启状态: " + content);
      }
    } else {
      logger->warn("无效的HTTP响应格式");
      throw std::runtime_error("无效的HTTP响应格式");
    }

    logger->info("initSocket start");
    logger->info("initSocket end");
    SkylineDebugInfo::InitSkylineDebugInfo(env, exports);
    SkylineShell::SkylineShell::Init(env, exports);
    Convert::RegisteInstanceType(env);
    // exports.Set("setConsole", Napi::Function::New(env, Logger::set_console));
    logger->info("return result");
  } catch (const std::exception& e) {
    logger->error("Error: {}", e.what());
    throw Napi::Error::New(env, e.what());
  }catch (...) {
    logger->error("Unknown error occurred during initialization.");
    throw Napi::Error::New(env, "Unknown error occurred during initialization.");
  }
  return exports;
}

NODE_API_MODULE(cmnative, Init)