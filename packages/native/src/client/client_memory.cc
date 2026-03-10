#include "client_memory.hh"

#include "../common/logger.hh"

#include <boost/asio.hpp>
#include <arpa/inet.h>
#include <array>
#include <cstdint>
#include <cstring>

using Logger::logger;

namespace SkylineClient {
namespace {
uint64_t hostToNetwork64(uint64_t value) {
  static const uint16_t one = 1;
  if (*reinterpret_cast<const uint8_t *>(&one) == 0) {
    return value;
  }
  const uint32_t high = htonl(static_cast<uint32_t>(value >> 32));
  const uint32_t low = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFULL));
  return (static_cast<uint64_t>(low) << 32) | high;
}

uint64_t networkToHost64(uint64_t value) { return hostToNetwork64(value); }
} // namespace

void ClientMemory::Init(Napi::Env env) {
  msgToServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_client2server", false);
  msgFromServer = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_server2client", false);

  boost::asio::ip::tcp::resolver resolver(ioContext);
  auto endpoints = resolver.resolve("127.0.0.1", std::to_string(3001));
  signalSocket = std::make_unique<boost::asio::ip::tcp::socket>(ioContext);
  boost::asio::connect(*signalSocket, endpoints);
  signalSocket->set_option(boost::asio::ip::tcp::no_delay(true));

  auto log = env.Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();
  log.Call(env.Global(), {Napi::String::New(env, "Connected signal socket for shared memory mode.")});
  logger->info("Client memory initialized with socket signaling");
}

void ClientMemory::sendMessage(std::string &&message, std::int64_t messageId) {
  if (!signalSocket || !signalSocket->is_open()) {
    throw std::runtime_error("Signal socket is not connected");
  }

  msgToServer->sendMessage(message);

  std::array<uint8_t, sizeof(uint32_t) + sizeof(uint64_t)> header{};
  const uint32_t signal = htonl(1);
  const uint64_t encodedMessageId = hostToNetwork64(static_cast<uint64_t>(messageId));
  std::memcpy(header.data(), &signal, sizeof(signal));
  std::memcpy(header.data() + sizeof(uint32_t), &encodedMessageId, sizeof(encodedMessageId));
  boost::asio::write(*signalSocket, boost::asio::buffer(header.data(), header.size()));
}

std::string ClientMemory::receiveMessage(std::int64_t *messageId) {
  if (!signalSocket || !signalSocket->is_open()) {
    throw std::runtime_error("Signal socket is not connected");
  }

  std::array<uint8_t, sizeof(uint32_t) + sizeof(uint64_t)> header{};
  boost::asio::read(*signalSocket, boost::asio::buffer(header.data(), header.size()));

  uint32_t signalNet = 0;
  std::memcpy(&signalNet, header.data(), sizeof(signalNet));
  const uint32_t signal = ntohl(signalNet);
  if (signal != 1) {
    logger->warn("Unexpected signal value from server: {}", signal);
  }

  uint64_t rawMessageId = 0;
  std::memcpy(&rawMessageId, header.data() + sizeof(uint32_t), sizeof(rawMessageId));
  if (messageId != nullptr) {
    *messageId = static_cast<int64_t>(networkToHost64(rawMessageId));
  }

  return msgFromServer->receiveMessage();
}

ClientMemory::~ClientMemory() {
  try {
    if (signalSocket && signalSocket->is_open()) {
      signalSocket->close();
    }
  } catch (...) {
  }
}
} // namespace SkylineClient
