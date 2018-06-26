#pragma once

#include "itransportlayer.h"
#include "tlsconfig.h"

// mbed
#include "TCPSocket.h"

#include <memory>
using SocketHandle = std::unique_ptr<TCPSocket>;

class NetworkStack;
class IStdInOut;

class SimpleTransport : public ITransportLayer
{
public:
    SimpleTransport(NetworkStack* _network, const TlsConfig& _config, IStdInOut* _log);
    virtual ~SimpleTransport();

    virtual bool connect(const char* server, size_t port) override;
    virtual bool disconnect() override;
    virtual size_t send(const uint8_t* data, size_t len) override;
    virtual size_t receive(uint8_t* data, size_t len) override;
    virtual void setTimeout(uint32_t timeoutMs) override;
    // make non-copyable C++11 style
    SimpleTransport(const SimpleTransport& other) = delete;
    SimpleTransport& operator=(const SimpleTransport&) = delete;

private:
    NetworkStack* network;
    SocketHandle socket;
    uint32_t socketTimeout;
    const TlsConfig& config;
    IStdInOut* log;
    char server[60];
    size_t port;
    int32_t error;
};
