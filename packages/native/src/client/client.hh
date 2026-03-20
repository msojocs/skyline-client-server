#ifndef __CLIENT_HH__
#define __CLIENT_HH__

#include <cstdint>
#include <string>
namespace SkylineClient {

class Client {
public:
    virtual void Init(std::string &address, int port) = 0;

    virtual bool IsConnected() = 0;
    
    // Send a message to the shared memory
    virtual void sendMessage(std::string &&message, std::int64_t messageId = 0) = 0;

    // Receive a message from the shared memory
    virtual std::string receiveMessage(std::int64_t *messageId = nullptr) = 0;

};
}
#endif // __CLIENT_HH__
