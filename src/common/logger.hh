#ifndef __LOGGER_HH__
#define __LOGGER_HH__

#include <spdlog/spdlog.h>
#include <napi.h>
namespace Logger {
  
  extern std::shared_ptr<spdlog::logger> logger;
  void Init();
}
#endif