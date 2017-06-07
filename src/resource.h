#pragma once

#include <string>
#include "Callback.h"

class Protocol;

class Resource
{
public:
    Resource(Protocol* _proto, const std::string& _topic);

    bool send(const std::string& data);
    bool sendCommandAck(const std::string& _command, const std::string& _code);
    bool recv(mbed::Callback<void(const char*)> callback);

private:
    Protocol* proto;
    const std::string topic;
    std::string publish;
    uint8_t packetId;
};
