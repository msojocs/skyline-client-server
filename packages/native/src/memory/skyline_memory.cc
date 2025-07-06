#include "skyline_memory.hh"
#include "manager.hh"
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <thread>
#include "../common/logger.hh"

using Logger::logger;
namespace SkylineMemory {
SharedMemoryCommunication::SharedMemoryCommunication(const std::string &name, bool create) {
    logger->debug("Initializing shared memory communication with name: {}", name);
    
    int size = 1024 * 1024;
    // Initialize the shared memory manager with the given name
    this->shared_memory = std::make_shared<SharedMemory::SharedMemoryManager>(name, create, size);
    SharedMemory::SharedMemoryHeader *header = static_cast<SharedMemory::SharedMemoryHeader *>(this->shared_memory->get_address());
    if (!header) {
        throw std::runtime_error("Failed to get shared memory address");
    }
    logger->debug("Initialized.");
}
SharedMemoryCommunication::~SharedMemoryCommunication() {
    // Clean up the shared memory manager
}

/**
 * 发送消息
 * 
 * 向循环内存区域写入数据
 */
void SharedMemoryCommunication::sendMessage(const std::string &message) {
    logger->debug("sendMessage");
    logger->debug("mem status: {}", !!this->shared_memory);
    if (!this->shared_memory || !this->shared_memory->get_address()) {
        logger->error("Shared memory not initialized or address is null");
        throw std::runtime_error("Shared memory not initialized or address is null");
    }
    logger->debug("Calculate start.");
    calculate:
    // 获取共享内存数据区的大小
    size_t memSize = this->shared_memory->get_size();
    logger->debug("Data memory size: {}", memSize);
    auto header = static_cast<SharedMemory::SharedMemoryHeader *>(this->shared_memory->get_address());
    int remainMemSize = header->data_end_offset - header->data_start_offset;
    int usedMemSize = 0;
    if (remainMemSize < 0) {
        // 长度小于0，end在前，start在后，绝对值是可用内存
        remainMemSize = -remainMemSize;
        usedMemSize = memSize - remainMemSize;
    }
    else {
        // 长度大于0，start在前，end在后，是已用内存
        usedMemSize = remainMemSize;
        remainMemSize = memSize - usedMemSize;
    }
    if (remainMemSize <= 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        goto calculate;
    }
    // 额外保留一个int大小，避免end在末尾但又不足以写入int的情况
    if (remainMemSize < message.size() + 2 * sizeof(int)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        goto calculate;
    }

    // 获取数据区起始地址
    auto dataMem = static_cast<char *>(this->shared_memory->get_address()) + sizeof(SharedMemory::SharedMemoryHeader); // 获取数据区起始地址
    auto mem = dataMem + header->data_end_offset; // 获取当前结束地址
    if (header->data_end_offset + sizeof(int) > memSize) {
        // 如果当前结束地址加上int大小超过了内存大小，则重置到起始位置
        header->data_end_offset = 0;
        mem = dataMem; // 重置到数据区起始地址
    }
    // 写入消息长度
    (*(int *)mem) = message.size();
    mem += sizeof(int); // 移动到消息内容位置
    // 写入消息内容
    if (header->data_end_offset + message.size() + sizeof(int) > memSize) {
        // 如果当前结束地址加上消息长度超过了内存大小，则需要分段写入
        size_t firstPartSize = memSize - header->data_end_offset - sizeof(int);
        std::memcpy(mem, message.data(), firstPartSize);
        // 写入剩余部分到起始位置
        std::memcpy(dataMem, message.data() + firstPartSize, message.size() - firstPartSize);
    }
    else {
        std::memcpy(mem, message.data(), message.size());
    }
    // 更新当前结束偏移
    header->data_end_offset = (header->data_end_offset + message.size() + sizeof(int)) % memSize;
    logger->debug("Message sent, size: {}, start offset: {}, end offset: {}, total size: {}", 
                  message.size(), header->data_start_offset, header->data_end_offset, memSize);
}
bool SharedMemoryCommunication::hasMessages() const {
    if (!this->shared_memory || !this->shared_memory->get_address()) {
        throw std::runtime_error("Shared memory not initialized or address is null");
    }
    auto header = static_cast<SharedMemory::SharedMemoryHeader *>(this->shared_memory->get_address());
    // 检查是否有消息
    return header->data_start_offset != header->data_end_offset;
}
std::string SharedMemoryCommunication::receiveMessage() {
    if (!this->shared_memory || !this->shared_memory->get_address()) {
        throw std::runtime_error("Shared memory not initialized or address is null");
    }
    auto header = static_cast<SharedMemory::SharedMemoryHeader *>(this->shared_memory->get_address());
    // 检查是否有消息
    if (header->data_start_offset == header->data_end_offset) {
        return ""; // 没有消息可读
    }
    
    // 获取数据区起始地址
    auto dataMem = static_cast<char *>(this->shared_memory->get_address()) + sizeof(SharedMemory::SharedMemoryHeader);
    auto mem = dataMem + header->data_start_offset; // 获取当前起始地址
    
    // 读取消息长度
    int messageSize = (*(int *)mem);
    mem += sizeof(int); // 移动到消息内容位置
    
    if (messageSize <= 0 || messageSize > this->shared_memory->get_size() - sizeof(SharedMemory::SharedMemoryHeader)) {
        logger->error("Invalid message size, size: {}", messageSize);
        logger->error("Invalid message size, start offset: {}, end offset: {}, total size: {}", 
                      header->data_start_offset, header->data_end_offset, this->shared_memory->get_size());
        throw std::runtime_error("Invalid message size, size: " + std::to_string(messageSize));
    }
    
    std::string message;
    message.resize(messageSize);
    
    if (header->data_start_offset + messageSize + sizeof(int) > this->shared_memory->get_size()) {
        logger->debug("Message size exceeds shared memory size, need to read in parts.");
        logger->debug("msg size: {}, start offset: {}, end offset: {}, total size: {}", 
                      messageSize, header->data_start_offset, header->data_end_offset, this->shared_memory->get_size());
        // 如果当前起始地址加上消息长度超过了内存大小，则需要分段读取
        size_t firstPartSize = this->shared_memory->get_size() - header->data_start_offset - sizeof(int);
        logger->debug("First part size: {}", firstPartSize);
        std::memcpy(&message[0], mem, firstPartSize);
        // 读取剩余部分到起始位置
        std::memcpy(&message[firstPartSize], dataMem, messageSize - firstPartSize);
    }
    else {
        std::memcpy(&message[0], mem, messageSize);
    }
    
    // 更新当前起始偏移
    header->data_start_offset = (header->data_start_offset + messageSize + sizeof(int)) % this->shared_memory->get_size();
    if (header->data_start_offset + sizeof(int) > this->shared_memory->get_size()) {
        // 如果当前起始地址加上int大小超过了内存大小，则重置到起始位置
        if (header->data_start_offset == header->data_end_offset) {
            header->data_end_offset = 0; // 如果起始和结束相同，重置结束偏移
        }
        header->data_start_offset = 0;
    }
    return message;
}
}