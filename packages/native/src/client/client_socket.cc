#include "client_socket.hh"
#include "../common/logger.hh"
#include "napi.h"
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