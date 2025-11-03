#include "server_socket.hh"
#include <boost/asio.hpp>
#include <cstdlib>
#include <thread>
#include <memory>
#include "../common/logger.hh"

using Logger::logger;
using boost::asio::ip::tcp;

namespace SkylineServer {
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
        socket->close();
        acceptor->close();
        io_context.stop();
        logger->info("Socket server stopped");
    } catch (const std::exception &e) {
        logger->error("Error stopping socket server: {}", e.what());
    }
}
void ServerSocket::sendMessage(std::string&& message) {
    try {
        if (socket && socket->is_open()) {
            uint32_t message_length = htonl(static_cast<uint32_t>(message.size()));
            // First send the length of the message
            boost::asio::write(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));
            // Then send the actual message
            boost::asio::write(*socket, boost::asio::buffer(message));
            logger->info("Sent message with length {}", message.size());
        } else {
            logger->error("Socket is not open. Cannot send message.");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    } catch (const std::exception &e) {
        logger->error("Error sending message: {}", e.what());
    }
}
std::string ServerSocket::receiveMessage() {
    try {
        if (socket && socket->is_open()) {
            uint32_t message_length = 0;
            boost::asio::read(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));
            message_length = ntohl(message_length);

            // Then read the actual message
            std::string message(message_length, '\0');
            boost::asio::read(*socket, boost::asio::buffer(message.data(), message_length));
            return message;
        } else {
            acceptor->accept(*socket);
            boost::asio::ip::tcp::no_delay option(true);
            socket->set_option(option);
            return "";
        }
    } catch (const std::exception &e) {
        logger->error("Error receiving message: {}", e.what());
        std::this_thread::sleep_for(std::chrono::seconds(1));
        exit(1);
        return "";
    }
}
}