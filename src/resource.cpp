#include "resource.h"
#include "resources.h"
#include <cstring>
#include "jsmn.h"
#include "mbed.h"
#include "jsondecode.h"

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
"\"commandID\":\"";

const std::string AckMiddle =
"\",\"code\":\"";

const std::string AckTime =
"\",\"time\":";

const std::string AckTail =
"}";

Resource::Resource(Resources* resources,
                   const std::string& _subtopic,
                   const std::string& _pubtopic)
    : proto(nullptr),
      subtopic("actuator/" + _subtopic),
      pubtopic("sensor/" + _pubtopic),
      subscriber(nullptr),
      publisher(nullptr)
{
    resources->registerResource(this);
}

void Resource::advertise(IPubSub* _proto)
{
    proto = _proto;
    publisher = std::make_unique<rtos::Thread>(osPriorityNormal, 0x400);
    publisher->start(mbed::callback(this, &Resource::publishThread));
}

void Resource::stopAdvertise()
{
    publisher.reset(nullptr);
    subscriber.reset(nullptr);
    proto = nullptr;
}

bool Resource::publish(const std::string& topic, const char* data, MessageDoneCallback doneCallback)
{
    message.clear();
    message.append(PubHeader);
    message.append(data);
    message.append(PubMiddle);
    message.append(std::to_string(time(nullptr)));
    message.append(PubTail);
    return proto->publish(reinterpret_cast<const uint8_t*>(topic.c_str()),
                          reinterpret_cast<const uint8_t*>(message.c_str()),
                          message.size(),
                          doneCallback);
}

bool Resource::acknowledge(const std::string& _command, int _code, MessageDoneCallback doneCallback)
{
    message.clear();
    message.append(AckHeader);
    message.append(_command);
    message.append(AckMiddle);
    message.append(std::to_string(_code));
    message.append(AckTime);
    message.append(std::to_string(time(nullptr)));
    message.append(AckTail);
    return proto->publish(reinterpret_cast<const uint8_t*>("ack"),
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
    subscriber = std::make_unique<rtos::Thread>(osPriorityNormal, 0x400);
    subscriber->start(mbed::callback(this, &Resource::subscribeThread));
}

void Resource::subscribeCallback(const uint8_t* data, size_t len)
{
    std::memcpy(subscribeContent, data, len);
    subscriber->signal_set(NEW_SUB_SIGNAL);
}

bool Resource::parseSubscription(std::string& commandID, std::string& data)
{
    JsonDecode message(subscribeContent, 16);
    bool parseOK = false;

    if(message)
    {
        size_t commandIDSize = 0;
        const char* commandIDStart = message.get("commandID", commandIDSize);
        size_t dataSize = 0;
        const char* dataStart = message.get("data", dataSize);

        if(((dataStart != subscribeContent) && (0 != dataSize))
            && ((commandIDStart != subscribeContent) && (0 != commandIDSize)))
        {
            commandID.clear();
            commandID.assign(commandIDStart, commandIDSize);
            data.clear();
            data.assign(dataStart, dataSize);
            parseOK = true;
        }
    }

    return parseOK;
}

void Resource::subscribeThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_SUB_SIGNAL);
        if(subscribed)
        {
            std::string commandID = "0";
            std::string data = "";
            int returnCode = 400; // Bad Request
            if(parseSubscription(commandID, data))
            {
                // this can be overridden by Resource implementation
                returnCode = handleCommand(data.c_str());
            }
            acknowledge(commandID, returnCode, mbed::callback(this, &Resource::ackDone));
            rtos::Thread::signal_wait(ACK_DONE_SIGNAL);
        }
    }
}

int Resource::handleCommand(const char* data)
{
    // Resource derivatives should implement command handling in overridden handleCommand
    return 404; // NOT_FOUND
}

void Resource::ackDone(bool status)
{
    subscriber->signal_set(ACK_DONE_SIGNAL);
}

void Resource::writeDone()
{
    subscriber->signal_set(SUB_DATA_DONE_SIGNAL);
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
    publisher->signal_set(PUBLISH_DONE_SIGNAL);
}

void Resource::publish()
{
    if(publisher)
    {
        publisher->signal_set(NEW_PUB_SIGNAL);
    }
}
