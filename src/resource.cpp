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

Resource::Resource(IPubSub* _proto,
                   const std::string& _subtopic,
                   const std::string& _pubtopic)
    : proto(_proto),
      subtopic(_subtopic),
      pubtopic(_pubtopic),
      acktopic(_subtopic + "ack")
{
    subscriber.start(mbed::callback(this, &Resource::subscribeThread));
    publisher.start(mbed::callback(this, &Resource::publishThread));
}

bool Resource::publish(const std::string& topic, const char* data, MessageDoneCallback doneCallback)
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

bool Resource::subscribe()
{
    return proto->subscribe(reinterpret_cast<const uint8_t*>(subtopic.c_str()),
                            mbed::callback(this, &Resource::subscribeDone),
                            mbed::callback(this, &Resource::subscribeCallback));
}

void Resource::subscribeDone(bool status)
{
    subscribed = status;
}

void Resource::subscribeCallback(const uint8_t* data, size_t len)
{
    command = std::make_unique<uint8_t[]>(len);
    std::memcpy(command.get(), data, len);
    subscriber.signal_set(NEW_SUB_SIGNAL);
}

void Resource::subscribeThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_SUB_SIGNAL);
        if(subscribed)
        {
            std::string commandName = "Toggle";
            std::string code = "200";
            acknowledge(acktopic, commandName, code, mbed::callback(this, &Resource::ackDone));
            rtos::Thread::signal_wait(ACK_DONE_SIGNAL);
            command.reset();
        }
    }
}

void Resource::ackDone(bool status)
{
    subscriber.signal_set(ACK_DONE_SIGNAL);
}

void Resource::writeDone()
{
    subscriber.signal_set(SUB_DATA_DONE_SIGNAL);
}

void Resource::publishThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_PUB_SIGNAL);
        publish(pubtopic, publishContent, mbed::callback(this, &Resource::publishDone));
        rtos::Thread::signal_wait(PUBLISH_DONE_SIGNAL);
    }
}

void Resource::publishDone(bool status)
{
    publisher.signal_set(PUBLISH_DONE_SIGNAL);
}

void Resource::publish()
{
    publisher.signal_set(NEW_PUB_SIGNAL);
}
