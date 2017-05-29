#pragma once

#include <string>

class Protocol;

class Resource
{
public:
    Resource(Protocol* _proto, const std::string& _topic);

    bool send(const std::string& data);
    bool recv();

private:
    Protocol* proto;
    const std::string topic;
    std::string publish;
    uint8_t packetId;
};
