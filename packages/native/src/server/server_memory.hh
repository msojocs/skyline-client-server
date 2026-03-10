#ifndef __SERVER_MEMORY_HH__
#define __SERVER_MEMORY_HH__

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "server.hh"
#include "../memory/skyline_memory.hh"
#include <boost/asio.hpp>

namespace SkylineServer {
    class ServerMemory : public Server {
    public:
        void Init(const Napi::CallbackInfo &info);
        ~ServerMemory();
        void sendMessage(std::string&& message, std::int64_t messageId = 0);
        std::string receiveMessage(std::int64_t *messageId = nullptr);
    private:
        std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgFromClient;
        std::shared_ptr<SkylineMemory::SharedMemoryCommunication> msgToClient;
        boost::asio::io_context ioContext;
        std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
        std::unique_ptr<boost::asio::ip::tcp::socket> signalSocket;
    };
}
#endif // __SERVER_MEMORY_HH__
