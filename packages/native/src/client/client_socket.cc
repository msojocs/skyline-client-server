#include "client_socket.hh"
#include "../common/logger.hh"
#include "napi.h"
#include <boost/asio.hpp>
#include <array>
#include <cstdint>
#include <memory>
#include <thread>

using boost::asio::ip::tcp;
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

static std::string serverAddress = "127.0.0.1";
static int serverPort = 3001;
void ClientSocket::Init(Napi::Env env) {
    auto log = env.Global().Get("console").As<Napi::Object>().Get("log").As<Napi::Function>();

    tcp::resolver resolver(io_context);
    auto endpoints =
        resolver.resolve(serverAddress, std::to_string(serverPort));

    socket = std::make_unique<tcp::socket>(io_context);
    log.Call(env.Global(), {Napi::String::New(env, "Connecting to server...")});
    boost::asio::connect(*socket, endpoints);
    
    // Enable TCP_NODELAY to reduce latency
    boost::asio::ip::tcp::no_delay option(true);
    socket->set_option(option);

    // Start a thread for the io_context
    std::thread([this]() {
        try {
            io_context.run();
        } catch (std::exception &e) {
            logger->error("IO context error: {}", e.what());
        }
    }).detach();

    // Handshake with server
    uint32_t handshake_value;
    for (int i=0; i<5; i++) {
        try  {
            boost::asio::read(*socket, boost::asio::buffer(&handshake_value, sizeof(handshake_value)));
            handshake_value = ntohl(handshake_value);
            if (handshake_value != 114514) {
                throw std::runtime_error("Invalid handshake from server: " + std::to_string(handshake_value));
            }
            logger->info("Successfully connected to server after handshake");
            log.Call(env.Global(), {Napi::String::New(env, "Connected to server.")});
            return;
        }
        catch(const std::exception& err) {
            logger->error("Handshake error (attempt {}/5): {}", i+1, err.what());
            log.Call(env.Global(), {Napi::String::New(env, std::string("Handshake error, retrying...") + err.what())});
            
            // Close and recreate socket for clean retry
            if (socket && socket->is_open()) {
                try {
                    socket->close();
                } catch(...) {}
            }
            socket = std::make_unique<tcp::socket>(io_context);
            
            // Wait a bit before retrying
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Try to reconnect
            try {
                boost::asio::connect(*socket, endpoints);
                socket->set_option(boost::asio::ip::tcp::no_delay(true));
            } catch(const std::exception& connect_err) {
                logger->error("Reconnect failed: {}", connect_err.what());
            }
        }
    }
    
    throw std::runtime_error("Failed to establish connection after 5 attempts");
    
}
void ClientSocket::sendMessage(std::string&& message, std::int64_t messageId) {
    if (socket && socket->is_open()) {
        logger->debug("Sending message with length: {}", message.size());
        uint32_t message_length = htonl(static_cast<uint32_t>(message.size()));
        uint64_t message_id = hostToNetwork64(static_cast<uint64_t>(messageId));
        std::array<boost::asio::const_buffer, 3> buffers = {
            boost::asio::buffer(&message_length, sizeof(message_length)),
            boost::asio::buffer(&message_id, sizeof(message_id)),
            boost::asio::buffer(message)
        };
        boost::asio::write(*socket, buffers);
    } else {
        logger->error("Socket is not open");
    }
}
std::string ClientSocket::receiveMessage(std::int64_t *messageId) {
    if (socket && socket->is_open()) {
        uint32_t message_length = 0;
        boost::asio::read(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));
        message_length = ntohl(message_length);

        uint64_t raw_message_id = 0;
        boost::asio::read(*socket, boost::asio::buffer(&raw_message_id, sizeof(raw_message_id)));
        if (messageId != nullptr) {
            *messageId = static_cast<int64_t>(networkToHost64(raw_message_id));
        }

        // Then read the actual message
        std::string message(message_length, '\0');
        boost::asio::read(*socket, boost::asio::buffer(message.data(), message_length));
        return message;
    } else {
        logger->error("Socket is not open");
        return "";
    }
}

ClientSocket::~ClientSocket() {}
} // namespace SkylineClient
