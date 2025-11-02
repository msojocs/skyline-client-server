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
        
        acceptor = std::make_shared<tcp::acceptor>(io_context, tcp::endpoint(tcp::v4(), port));
        socket = std::make_shared<tcp::socket>(io_context);
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
void ServerSocket::sendMessage(const std::string &message) {
    try {
        if (socket && socket->is_open()) {
            // Then send the actual message
            boost::asio::write(*socket, boost::asio::buffer(message + "\n"));
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
            boost::asio::streambuf buf;
            boost::asio::read_until(*socket, buf, "\n");
            std::string data{std::istreambuf_iterator<char>(&buf),
                            std::istreambuf_iterator<char>()};
            return data;
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