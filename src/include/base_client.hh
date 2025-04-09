#ifndef __BASE_CLIENT_HH__
#define __BASE_CLIENT_HH__
#include <napi.h>
#include <nlohmann/json.hpp>
namespace Skyline {

  class BaseClient {
  protected:
    std::string m_clientTag;
    std::string sendToServer(const std::string &action, nlohmann::json &data);
  private:
  };
}

#endif