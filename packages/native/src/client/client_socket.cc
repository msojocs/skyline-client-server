#include "client_socket.hh"
#include "../common/logger.hh"
#include <boost/asio.hpp>
#include <memory>
#include <thread>

using boost::asio::ip::tcp;
using Logger::logger;

namespace SkylineClient {
static std::string serverAddress = "127.0.0.1";
static int serverPort = 3001;
void ClientSocket::Init(Napi::Env env) {

    tcp::resolver resolver(io_context);
    auto endpoints =
        resolver.resolve(serverAddress, std::to_string(serverPort));

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
        uint32_t message_length = htonl(static_cast<uint32_t>(message.size()));
        // First send the length of the message
        boost::asio::write(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));
        // Then send the actual message
        boost::asio::write(*socket, boost::asio::buffer(message));
    } else {
        logger->error("Socket is not open");
    }
}
std::string ClientSocket::receiveMessage() {
    if (socket && socket->is_open()) {
        uint32_t message_length = 0;
        boost::asio::read(*socket, boost::asio::buffer(&message_length, sizeof(message_length)));
        message_length = ntohl(message_length);

        // Then read the actual message
        std::vector<char> buffer(message_length);
        boost::asio::read(*socket, boost::asio::buffer(buffer.data(), message_length));

        std::string message(buffer.data(), message_length);
        return message;
    } else {
        logger->error("Socket is not open");
        return "";
    }
}

ClientSocket::~ClientSocket() {}
} // namespace SkylineClient