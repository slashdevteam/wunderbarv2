#pragma once

#include <string>
#include <memory>
#include "ipubsub.h"
#include "Thread.h"

const uint32_t MQTT_MSG_PAYLOAD_SIZE = 200;

class IPubSub;

class Resource
{
public:
    Resource(IPubSub* _proto,
             const std::string& _subtopic,
             const std::string& _pubtopic);

    bool subscribe();
    void publish();
    bool acknowledge(const std::string& topic,
                     const std::string& _command,
                     const std::string& _code,
                     MessageDoneCallback doneCallback);
    void writeDone();

private:
    bool publish(const std::string& topic,
                 const char* data,
                 MessageDoneCallback doneCallback);
    void publishThread();
    void publishDone(bool status);
    void subscribeThread();
    void subscribeCallback(const uint8_t* data, size_t len);
    void subscribeDone(bool status);
    void ackDone(bool status);

protected:
    char publishContent[MQTT_MSG_PAYLOAD_SIZE];
    char subscribeContent[MQTT_MSG_PAYLOAD_SIZE];

private:
    const int32_t NEW_SUB_SIGNAL       = 0x1;
    const int32_t ACK_DONE_SIGNAL      = 0x2;
    const int32_t NEW_PUB_SIGNAL       = 0x4;
    const int32_t PUBLISH_DONE_SIGNAL  = 0x8;
    const int32_t SUB_DATA_DONE_SIGNAL = 0x10;

    std::string message;
    IPubSub* proto;

    const std::string subtopic;
    const std::string pubtopic;
    const std::string acktopic;
    rtos::Thread subscriber;
    rtos::Thread publisher;
    std::unique_ptr<uint8_t[]> command;
    volatile bool subscribed;
};
