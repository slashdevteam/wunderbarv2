#pragma once

#include "Callback.h"

class NetworkStack;

class ITransportLayer
{
public:
    ITransportLayer() = default;
    virtual ~ITransportLayer() = default;
    virtual bool connect(const char* server, size_t port) = 0;
    virtual bool disconnect() = 0;
    virtual size_t send(const uint8_t* data, size_t len) = 0;
    virtual size_t receive(uint8_t* data, size_t len) = 0;
    virtual void setTimeout(uint32_t timeoutMs) = 0;

    // make non-copyable C++11 style
    ITransportLayer(const ITransportLayer& other) = delete;
    ITransportLayer& operator=(const ITransportLayer&) = delete;
};
