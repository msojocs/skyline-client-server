#ifndef __BASE_CLIENT_HH__
#define __BASE_CLIENT_HH__
#include <napi.h>
#include <nlohmann/json.hpp>
namespace Skyline {

class BaseClient {
protected:
  std::string m_instanceId;
  virtual Napi::Value getInstanceId(const Napi::CallbackInfo &info) = 0;
private:
};

} // namespace Skyline

#endif