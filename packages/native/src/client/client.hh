#ifndef __CLIENT_HH__
#define __CLIENT_HH__

#include "napi.h"
namespace SkylineClient {

class Client {
public:
    virtual void Init(Napi::Env env) = 0;
    
    // Send a message to the shared memory
    virtual void sendMessage(const std::string &message) = 0;

    // Receive a message from the shared memory
    virtual std::string receiveMessage(const std::string &name) = 0;

};
}
#endif // __CLIENT_HH__