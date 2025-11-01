#ifndef __CLIENT_SOCKET_HH__
#define __CLIENT_SOCKET_HH__
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <string>
#include <napi.h>
#include "client.hh"
#include <boost/asio.hpp>

namespace SkylineClient {
    

using boost::asio::ip::tcp;
class ClientSocket : public Client {
    public:
    void Init(Napi::Env env);
    ~ClientSocket();
    void sendMessage(const std::string &message);
    std::string receiveMessage();

    private:
    boost::asio::io_context io_context;
    std::shared_ptr<tcp::socket> socket;
};
}

#endif // __SOCKET_CLIENT_HH__