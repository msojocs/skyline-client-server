#include "server_socket.hh"
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <memory>
#include "../common/logger.hh"
#include "../common/snowflake.hh"
#include "../common/convert.hh"


using Logger::logger;
using boost::asio::ip::tcp;

namespace SkylineServer {
void ServerSocket::Init(Napi::Env env) {
    try {
        int port = 12345; // Default port
        
        acceptor = std::make_shared<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), port));
        socket = std::make_shared<tcp::socket>(io_context);

        // Start accepting connections
        acceptor->async_accept(*socket, [this](const boost::system::error_code &error) {
            if (!error) {
                logger->info("Client connected to socket server");
            } else {
                logger->error("Error accepting connection: {}", error.message());
            }
        });

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
void ServerSocket::sendMessage(const std::string &message) {
    try {
        if (socket && socket->is_open()) {
            // Send the message length first
            uint32_t message_length = htonl(static_cast<uint32_t>(message.size()));
            boost::asio::write(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));

            // Then send the actual message
            boost::asio::write(*socket, boost::asio::buffer(message));
            logger->info("Sent message of length {}", message.size());
        } else {
            logger->error("Socket is not open. Cannot send message.");
        }
    } catch (const std::exception &e) {
        logger->error("Error sending message: {}", e.what());
    }
}
std::string ServerSocket::receiveMessage(const std::string &name) {
    try {
        if (socket && socket->is_open()) {
            // Read the message length first
            uint32_t message_length = 0;
            boost::asio::read(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));
            message_length = ntohl(message_length);

            // Then read the actual message
            std::vector<char> buffer(message_length);
            boost::asio::read(*socket, boost::asio::buffer(buffer.data(), message_length));

            std::string message(buffer.data(), message_length);
            logger->info("Received message of length {}", message.size());
            return message;
        } else {
            logger->error("Socket is not open. Cannot receive message.");
            return "";
        }
    } catch (const std::exception &e) {
        logger->error("Error receiving message: {}", e.what());
        return "";
    }
}
}