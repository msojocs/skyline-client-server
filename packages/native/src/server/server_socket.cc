#include "server_socket.hh"
#include <boost/asio.hpp>
#include <boost/asio/write.hpp>
#include <array>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <memory>
#include "../common/logger.hh"

using Logger::logger;
using boost::asio::ip::tcp;

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

void ServerSocket::Init(const Napi::CallbackInfo &info) {
    try {
        auto env = info.Env();
        auto port = info[1].As<Napi::Number>().Int32Value();
        
        acceptor = std::make_unique<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), port));
        socket = std::make_unique<tcp::socket>(io_context);
        logger->info("Socket server listening on *:{}", port);

        // Run the IO context in a separate thread
        std::thread([this]() {
            try {
                io_context.run();
            } catch (const std::exception &e) {
                logger->error("IO context error: {}", e.what());
            }
        }).detach();
    } catch (const std::exception &e) {
        logger->error("Failed to start socket server: {}", e.what());
    }
}
ServerSocket::~ServerSocket() {
    try {
        if (socket && socket->is_open()) {
            socket->close();
        }
        if (acceptor && acceptor->is_open()) {
            acceptor->close();
        }
        io_context.stop();
        logger->info("Socket server stopped");
    } catch (const std::exception &e) {
        logger->error("Error stopping socket server: {}", e.what());
    }
}
void ServerSocket::sendMessage(std::string&& message, std::int64_t messageId) {
    try {
        if (socket && socket->is_open()) {
            std::array<uint8_t, sizeof(uint32_t) + sizeof(uint64_t)> header{};
            const uint32_t message_length = htonl(static_cast<uint32_t>(message.size()));
            const uint64_t message_id = hostToNetwork64(static_cast<uint64_t>(messageId));
            std::memcpy(header.data(), &message_length, sizeof(message_length));
            std::memcpy(header.data() + sizeof(uint32_t), &message_id, sizeof(message_id));
            std::array<boost::asio::const_buffer, 2> buffers = {
                boost::asio::buffer(header.data(), header.size()),
                boost::asio::buffer(message)
            };
            boost::asio::write(*socket, buffers);
            logger->debug("Sent message with length {}", message.size());
        } else {
            logger->error("Socket is not open. Cannot send message.");
        }
    } catch (const std::exception &e) {
        logger->error("Error sending message: {}", e.what());
    }
}
std::string ServerSocket::receiveMessage(std::int64_t *messageId) {
    try {
        if (socket && socket->is_open()) {
            std::array<uint8_t, sizeof(uint32_t) + sizeof(uint64_t)> header{};
            boost::asio::read(*socket, boost::asio::buffer(header.data(), header.size()));

            uint32_t message_length_net = 0;
            std::memcpy(&message_length_net, header.data(), sizeof(message_length_net));
            const uint32_t message_length = ntohl(message_length_net);

            uint64_t raw_message_id = 0;
            std::memcpy(&raw_message_id, header.data() + sizeof(uint32_t), sizeof(raw_message_id));
            if (messageId != nullptr) {
                *messageId = static_cast<int64_t>(networkToHost64(raw_message_id));
            }

            // Then read the actual message
            std::string message(message_length, '\0');
            boost::asio::read(*socket, boost::asio::buffer(message.data(), message_length));
            return message;
        } else {
            // 关闭上一次连接残留的 socket，重新创建后再 accept
            if (socket) {
                boost::system::error_code ec;
                socket->close(ec);
            }
            socket = std::make_unique<tcp::socket>(io_context);
            logger->info("Waiting for client connection...");
            acceptor->accept(*socket);
            boost::asio::ip::tcp::no_delay option(true);
            socket->set_option(option);
            logger->info("Client connected");
            // 握手数据
            uint32_t message_length = htonl(static_cast<uint32_t>(114514));
            boost::asio::write(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));
            return "";
        }
    } catch (const std::exception &e) {
        logger->error("Error receiving message: {}", e.what());
        // 关闭 socket，使下次调用进入 accept 分支等待新连接
        if (socket) {
            boost::system::error_code ec;
            socket->close(ec);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
        throw; // 重新抛出异常，让调用者处理
    }
}
}
