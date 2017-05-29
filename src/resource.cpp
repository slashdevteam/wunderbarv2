#include "resource.h"
#include "protocol.h"

const std::string PubHeader =
"{"
"    \"data\" : {";

const std::string PubMiddle =
"   },"
"    \"time\" :";

const std::string PubTail =
"}";

const std::string CommandAckJson =
"{"
"   \"commandID\": \"%s\","
"   \"code\": %d,"
"    \"time\" : %lld"
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
    return proto->publish(topic.c_str(), publish.c_str(), publish.length(), packetId);
}

bool Resource::recv()
{
    return false;
}
