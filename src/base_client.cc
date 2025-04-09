#include "include/base_client.hh"
#include "napi.h"
#include "websocket.hh"
#include <spdlog/spdlog.h>
#include <windows.h>
namespace Skyline {


std::string BaseClient::sendToServer(const std::string &action, nlohmann::json &data) {
  // 发送消息到 WebSocket
  return WebSocket::sendMessageSync(m_clientTag, action, data);
}
} // namespace SkylineShell