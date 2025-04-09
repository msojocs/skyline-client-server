#ifndef __CONVERT_HH__
#define __CONVERT_HH__
#include <napi.h>
#include <nlohmann/json.hpp>

namespace Convert {
  nlohmann::json convertValue2Json(const Napi::Value &value);
nlohmann::json convertObject2Json(const Napi::Value &value);

} // namespace Convert

#endif