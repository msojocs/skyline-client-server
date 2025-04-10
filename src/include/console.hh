#ifndef __CONSELE_HH__
#define __CONSELE_HH__

#include <napi.h>
namespace Logger {
  Napi::Value set_console(const Napi::CallbackInfo &info);
  void log(const std::string& message);
  void log(const char* format, ...);
}
#endif