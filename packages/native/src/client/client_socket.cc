#include "client_socket.hh"
#include "../common/logger.hh"
#include <boost/asio.hpp>
#include <cstdint>
#include <memory>
#include <thread>

using boost::asio::ip::tcp;
using Logger::logger;

namespace SkylineClient {
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
    std::thread([&]() {
        try {
            io_context.run();
        } catch (std::exception &e) {
            logger->error("IO context error: {}", e.what());
        }
    }).detach();

    uint32_t start;
    boost::asio::read(*socket, boost::asio::buffer(&start, sizeof(start)));
    start = ntohl(start);
    if (start != 114514) {
        throw std::runtime_error("Invalid handshake from server");
    }
    log.Call(env.Global(), {Napi::String::New(env, "Connected to server.")});
    
}
void ClientSocket::sendMessage(std::string&& message) {
    if (socket && socket->is_open()) {
        logger->debug("Sending message: {}", message);
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