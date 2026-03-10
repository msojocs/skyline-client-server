#include "server_memory.hh"

#include "../common/logger.hh"

#include <array>
#include <cstdint>
#include <cstring>

using Logger::logger;

namespace SkylineServer {
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

void ServerMemory::Init(const Napi::CallbackInfo &info) {
  try {
    auto port = info[1].As<Napi::Number>().Int32Value();
    msgToClient = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_server2client", true);
    msgFromClient = std::make_shared<SkylineMemory::SharedMemoryCommunication>("skyline_client2server", true);

    acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(
        ioContext,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
    signalSocket = std::make_unique<boost::asio::ip::tcp::socket>(ioContext);
    logger->info("Shared memory server initialized with socket signaling on port {}", port);
  } catch (const std::exception &e) {
    logger->error("Failed to initialize shared memory server: {}", e.what());
    throw;
  }
}

ServerMemory::~ServerMemory() {
  try {
    if (signalSocket && signalSocket->is_open()) {
      signalSocket->close();
    }
    if (acceptor && acceptor->is_open()) {
      acceptor->close();
    }
    ioContext.stop();
  } catch (const std::exception &e) {
    logger->error("Error stopping shared memory server: {}", e.what());
  }
}

void ServerMemory::sendMessage(std::string&& message, std::int64_t messageId) {
  try {
    if (!signalSocket || !signalSocket->is_open()) {
      logger->error("Signal socket is not connected. Cannot send message.");
      return;
    }

    msgToClient->sendMessage(message);

    std::array<uint8_t, sizeof(uint32_t) + sizeof(uint64_t)> header{};
    const uint32_t signal = htonl(1);
    const uint64_t encodedMessageId = hostToNetwork64(static_cast<uint64_t>(messageId));
    std::memcpy(header.data(), &signal, sizeof(signal));
    std::memcpy(header.data() + sizeof(uint32_t), &encodedMessageId, sizeof(encodedMessageId));
    boost::asio::write(*signalSocket, boost::asio::buffer(header.data(), header.size()));
  } catch (const std::exception &e) {
    logger->error("Error sending message: {}", e.what());
  }
}

std::string ServerMemory::receiveMessage(std::int64_t *messageId) {
  try {
    if (!signalSocket || !signalSocket->is_open()) {
      acceptor->accept(*signalSocket);
      signalSocket->set_option(boost::asio::ip::tcp::no_delay(true));
      logger->info("Signal socket client connected");
      return "";
    }

    std::array<uint8_t, sizeof(uint32_t) + sizeof(uint64_t)> header{};
    boost::asio::read(*signalSocket, boost::asio::buffer(header.data(), header.size()));

    uint32_t signalNet = 0;
    std::memcpy(&signalNet, header.data(), sizeof(signalNet));
    const uint32_t signal = ntohl(signalNet);
    if (signal != 1) {
      logger->warn("Unexpected signal value from client: {}", signal);
    }

    uint64_t rawMessageId = 0;
    std::memcpy(&rawMessageId, header.data() + sizeof(uint32_t), sizeof(rawMessageId));
    if (messageId != nullptr) {
      *messageId = static_cast<int64_t>(networkToHost64(rawMessageId));
    }

    return msgFromClient->receiveMessage();
  } catch (const std::exception &e) {
    logger->error("Error receiving message: {}", e.what());
    throw;
  }
}
} // namespace SkylineServer
