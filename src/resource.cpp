#include "resource.h"
#include "protocol.h"

const std::string PubHeader =
"{"
"\"data\":{";

const std::string PubMiddle =
"},"
"\"time\":";

const std::string PubTail =
"}";

const std::string AckHeader =
"{"
"\"commandID\":";

const std::string AckMiddle =
"\"code\":";

const std::string AckTime =
"\"time\":";

const std::string AckTail =
"}";

Resource::Resource(Protocol* _proto, const std::string& _topic)
    : proto(_proto),
      topic(_topic),
      packetId(0)
{}


bool Resource::send(const std::string& data)
{
    packetId++;
    publish.clear();
    publish.append(PubHeader);
    publish.append(data);
    publish.append(PubMiddle);
    publish.append(std::to_string(10)); // epoch
    publish.append(PubTail);
    return proto->publish(topic, publish, packetId);
}

bool Resource::sendCommandAck(const std::string& _command, const std::string& _code)
{
    packetId++;
    publish.clear();
    publish.append(AckHeader);
    publish.append(_command);
    publish.append(AckMiddle);
    publish.append(_code);
    publish.append(AckTime);
    publish.append(std::to_string(13)); // epoch
    publish.append(AckTail);
    return proto->publish("ack", publish, packetId);
}

bool Resource::recv(mbed::Callback<void(const char*)> callback)
{
    packetId++;
    return proto->subscribe(topic, callback, packetId);
}
