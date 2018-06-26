#include "simpletransport.h"
#include "istdinout.h"
#include <cstring>

#include <memory>
// mbed
#include "Thread.h"

const int32_t DEFAULT_SOCKET_TIMEOUT = 3000;

SimpleTransport::SimpleTransport(NetworkStack* _network, const TlsConfig& _config, IStdInOut* _log)
    : network(_network),
      socket(nullptr),
      socketTimeout(DEFAULT_SOCKET_TIMEOUT),
      config(_config),
      log(_log),
      error(0)
{

}

SimpleTransport::~SimpleTransport()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    disconnect();
}

void SimpleTransport::setTimeout(uint32_t timeoutMs)
{
    socketTimeout = timeoutMs;
    if(socket)
    {
        socket->set_timeout(socketTimeout);
    }
}

bool SimpleTransport::connect(const char* _server, size_t _port)
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    socket = std::make_unique<TCPSocket>(network);
    socket->set_timeout(socketTimeout);
    error = socket->connect(_server, _port);
    if(error)
    {
        log->printf("Socket connect fail - %d\r\n", error);
        socket->close();
        return false;
    }

    return true;
}

bool SimpleTransport::disconnect()
{
    error = 0;
    socket.reset(nullptr);
    return true;
}

size_t SimpleTransport::send(const uint8_t* data, size_t len)
{
    int sent = 0;

    sent = socket->send(data, len);
    if(sent < 0)
    {
        error = sent;
        sent = 0;
    }

    return sent;
}

size_t SimpleTransport::receive(uint8_t* data, size_t len)
{
    int received = 0;

    received = socket->recv(data, len);
    if(received < 0)
    {
        return 0;
    }

    return received;
}
