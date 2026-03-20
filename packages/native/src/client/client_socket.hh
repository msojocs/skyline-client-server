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
    void Init(std::string &, int);
    bool IsConnected();
    ~ClientSocket();
    void sendMessage(std::string&& message, std::int64_t messageId = 0);
    std::string receiveMessage(std::int64_t *messageId = nullptr);

    private:
    boost::asio::io_context io_context;
    std::unique_ptr<tcp::socket> socket;
    bool is_connected = false;
    std::string server_address;
    int server_port;
};
}

#endif // __SOCKET_CLIENT_HH__
