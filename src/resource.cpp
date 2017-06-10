#include "resource.h"

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

Resource::Resource(IPubSub* _proto)
    : proto(_proto)
{}


bool Resource::publish(const std::string& topic, const std::string& data, MessageDoneCallback doneCallback)
{
    message.clear();
    message.append(PubHeader);
    message.append(data);
    message.append(PubMiddle);
    message.append(std::to_string(10)); // epoch @TODO: get from RTC/Transport layer
    message.append(PubTail);
    return proto->publish(reinterpret_cast<const uint8_t*>(topic.c_str()),
                          reinterpret_cast<const uint8_t*>(message.c_str()),
                          message.size(),
                          doneCallback);
}

bool Resource::acknowledge(const std::string& topic, const std::string& _command, const std::string& _code, MessageDoneCallback doneCallback)
{
    message.clear();
    message.append(AckHeader);
    message.append(_command);
    message.append(AckMiddle);
    message.append(_code);
    message.append(AckTime);
    message.append(std::to_string(13)); // epoch
    message.append(AckTail);
    return proto->publish(reinterpret_cast<const uint8_t*>(topic.c_str()),
                              reinterpret_cast<const uint8_t*>(message.c_str()),
                              message.size(),
                              doneCallback);
}

bool Resource::subscribe(const std::string& topic, MessageDoneCallback doneCallback, MessageDataCallback datacallback)
{
    return proto->subscribe(reinterpret_cast<const uint8_t*>(topic.c_str()),
                            doneCallback,
                            datacallback);
}
