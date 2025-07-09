#ifndef __SKYLINE_MEMORY_HH__
#define __SKYLINE_MEMORY_HH__
#include "manager.hh"
#include <memory>
#include <string>

namespace SkylineMemory {
class SharedMemoryCommunication {
public:
    SharedMemoryCommunication(const std::string &name, bool create);

    ~SharedMemoryCommunication();

    // Send a message to the shared memory
    void sendMessage(const std::string &message);

    // Receive a message from the shared memory
    std::string receiveMessage(const std::string &name);

    // Check if there are messages in the queue
    bool hasMessages() const;
    // 用于发送消息后通知的句柄
#ifdef _WIN32
    HANDLE file_notify = nullptr;
#elif __linux__
    sem_t *file_notify = nullptr;
#endif
private:
    // Internal data structures for managing shared memory
    // This could include a queue, mutexes, etc.
    // For simplicity, we will use a string as a placeholder
    std::shared_ptr<SharedMemory::SharedMemoryManager> shared_memory;
};
}

#endif // MEMORY_HH