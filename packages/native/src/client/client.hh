#ifndef __CLIENT_HH__
#define __CLIENT_HH__

#include <cstdint>
#include "napi.h"
namespace SkylineClient {

class Client {
public:
    virtual void Init(Napi::Env env) = 0;
    
    // Send a message to the shared memory
    virtual void sendMessage(std::string &&message, std::int64_t messageId = 0) = 0;

    // Receive a message from the shared memory
    virtual std::string receiveMessage(std::int64_t *messageId = nullptr) = 0;

};
}
#endif // __CLIENT_HH__
