#include "client_socket.hh"
#include "../common/logger.hh"
#include <boost/asio.hpp>
#include <memory>
#include <thread>

using boost::asio::ip::tcp;
using Logger::logger;

namespace SkylineClient {
void ClientSocket::Init(Napi::Env env) {

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", std::to_string(3001));

    socket = std::make_shared<tcp::socket>(io_context);
    boost::asio::connect(*socket, endpoints);
    
    // Enable TCP_NODELAY to reduce latency
    boost::asio::ip::tcp::no_delay option(true);
    socket->set_option(option);

    // Start a thread for the io_context
    std::thread([&]() {
        try {
            io_context.run();
        } catch (std::exception &e) {
            logger->error("IO context error: {}", e.what());
        }
    }).detach();
}
void ClientSocket::sendMessage(const std::string &message) {
    if (socket && socket->is_open()) {
        // Then send the actual message
        boost::asio::write(*socket, boost::asio::buffer(message + "\n"));
    } else {
        logger->error("Socket is not open");
    }
}
std::string ClientSocket::receiveMessage() {
    if (socket && socket->is_open()) {
        boost::asio::streambuf buf;
        boost::asio::read_until(*socket, buf, "\n");
        std::string data{std::istreambuf_iterator<char>(&buf),
                        std::istreambuf_iterator<char>()};
        return data;
    } else {
        logger->error("Socket is not open");
        return "";
    }
}

ClientSocket::~ClientSocket() {}
} // namespace SkylineClient