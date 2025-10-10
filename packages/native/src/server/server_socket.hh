#ifndef __SERVER_SOCKET_HH__
#define __SERVER_SOCKET_HH__
#include "server.hh"
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
namespace SkylineServer {
    class ServerSocket : public Server {
    public:
        void Init(Napi::Env env);
        ~ServerSocket();
        void sendMessage(const std::string &message);
        std::string receiveMessage(const std::string &name);
    private:
        boost::asio::io_context io_context;
        std::shared_ptr<tcp::acceptor> acceptor;
        std::shared_ptr<tcp::socket> socket;
    };
}
#endif // __SERVER_SOCKET_HH__