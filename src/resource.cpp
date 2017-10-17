#include "resource.h"
#include "resources.h"
#include <cstring>
#include "jsmn.h"
#include "mbed.h"
#include "jsondecode.h"

const char PubHeader[] =
"{"
"\"data\":{";

const char PubMiddle[] =
"},"
"\"time\":";

const char PubTail[] =
"}";

const char AckHeader[] =
"{"
"\"commandID\":\"";

const char AckMiddle[] =
"\",\"code\":\"";

const char AckTime[] =
"\",\"time\":";

const char AckTail[] =
"}";

Resource::Resource(Resources* resources,
                   const std::string& _subtopic,
                   const std::string& _pubtopic,
                   IStdInOut& _log)
    : proto(nullptr),
      subtopic("actuator/" + _subtopic),
      pubtopic("sensor/" + _pubtopic),
      pubSub(nullptr),
      log(_log)
{
    resources->registerResource(this);
}

Resource::~Resource()
{
    revoke();
}

void Resource::advertise(IPubSub* _proto)
{
    proto = _proto;
    pubSub = std::make_unique<rtos::Thread>(osPriorityNormal, 0xA00, nullptr, pubtopic.c_str());
    pubSub->start(mbed::callback(this, &Resource::pubSubThread));
}

void Resource::revoke()
{
    pubSub.reset(nullptr);
    proto = nullptr;
}

void Resource::pubSubThread()
{
    while(1)
    {
        osEvent msg = msgQueue.get();
        if(osEventMessage == msg.status)
        {
            MessageTuple& message = *reinterpret_cast<MessageTuple*>(msg.value.p);
            MessageType type = std::get<MessageType>(message);
            switch(type)
            {
                case SUB_REQ:
                    proto->subscribe(reinterpret_cast<const uint8_t*>(subtopic.c_str()),
                                     mbed::callback(this, &Resource::subscribeDone),
                                     mbed::callback(this, &Resource::subscribeCallback));
                    rtos::Thread::signal_wait(SUB_ACK_SIGNAL);
                    break;
                case PUB_REQ:
                    {
                        proto->publish(reinterpret_cast<const uint8_t*>(pubtopic.c_str()),
                                       reinterpret_cast<const uint8_t*>(std::get<Message>(message).data()),
                                       std::get<size_t>(message),
                                       mbed::callback(this, &Resource::publishDone));
                        rtos::Thread::signal_wait(PUBLISH_DONE_SIGNAL);
                    }
                    break;
                case ACK_REQ:
                    {
                        proto->publish(reinterpret_cast<const uint8_t*>("ack"),
                                       reinterpret_cast<const uint8_t*>(std::get<Message>(message).data()),
                                       std::get<size_t>(message),
                                       mbed::callback(this, &Resource::ackDone));
                        rtos::Thread::signal_wait(ACK_DONE_SIGNAL);
                    }
                    break;
                case SUB_INCOMING:
                    handleSubscription(message);
                    break;
                default:
                    break;
            }
            msgStorage.free(&message);
        }
    }
}

void Resource::acknowledge(const char* commandId, int code)
{
    MessageTuple* message = msgStorage.alloc();
    if(message)
    {
        std::get<MessageType>(*message) = ACK_REQ;
        char* content = std::get<Message>(*message).data();

        size_t written = std::snprintf(content, MQTT_MSG_PAYLOAD_SIZE, AckHeader);
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, commandId);
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, AckMiddle);
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, "%d", code);
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, AckTime);
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, "%ld", time(nullptr));
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, AckTail);
        std::get<size_t>(*message) = written;
        msgQueue.put(message);
        log.printf("%s: commandId: %s returned code: %d\r\n", subtopic.c_str(), commandId, code);
    }
}

void Resource::subscribe()
{
    MessageTuple* message = msgStorage.alloc();
    if(message)
    {
        std::get<MessageType>(*message) = SUB_REQ;
        msgQueue.put(message);
    }
}

void Resource::publish(DataFiller fillData, const uint8_t* extData)
{
    MessageTuple* message = msgStorage.alloc();
    if(message)
    {
        std::get<MessageType>(*message) = PUB_REQ;
        char* content = std::get<Message>(*message).data();

        size_t written = std::snprintf(content, MQTT_MSG_PAYLOAD_SIZE, PubHeader);
        if(fillData)
        {
            written += fillData(content + written, MQTT_MSG_PAYLOAD_SIZE - written, extData);
        }
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, PubMiddle);
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, "%ld", time(nullptr));
        written += std::snprintf(content + written, MQTT_MSG_PAYLOAD_SIZE - written, PubTail);
        std::get<size_t>(*message) = written;
        msgQueue.put(message);
    }
}

void Resource::subscribeDone(bool status)
{
    subscribed = status;
    if(pubSub)
    {
        pubSub->signal_set(SUB_ACK_SIGNAL);
    }
}

void Resource::subscribeCallback(const uint8_t* data, size_t len)
{
    if(len <= MQTT_MSG_PAYLOAD_SIZE)
    {
        MessageTuple* message = msgStorage.alloc();
        if(message)
        {
            std::get<MessageType>(*message) = SUB_INCOMING;
            std::get<size_t>(*message) = len;
            std::memcpy(std::get<Message>(*message).data(), data, len);
            msgQueue.put(message);
        }
    }
}

bool Resource::parseSubscription(const char* subscribeContent, std::string& commandID, std::string& data)
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

void Resource::handleSubscription(MessageTuple& subMsg)
{
    if(subscribed)
    {
        std::string commandID = "0";
        std::string data = "";
        if(parseSubscription(std::get<Message>(subMsg).data(), commandID, data))
        {
            // this can be overridden by Resource implementation
            handleCommand(commandID.c_str(), data.c_str());
        }
        else
        {
            acknowledge(commandID.c_str(), 400); // Bad Request
        }
    }
}

void Resource::handleCommand(const char* id, const char* data)
{
    // Resource derivatives should implement command handling in overridden handleCommand
    // 404 - NOT_FOUND
    acknowledge(id, 404);
}

void Resource::ackDone(bool status)
{
    if(pubSub)
    {
        pubSub->signal_set(ACK_DONE_SIGNAL);
    }
}

void Resource::publishDone(bool status)
{
    if(pubSub)
    {
        pubSub->signal_set(PUBLISH_DONE_SIGNAL);
    }
}
