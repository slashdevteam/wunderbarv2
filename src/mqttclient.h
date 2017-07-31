#pragma once

#include "resource.h"
#include "Thread.h"

#include "iblegateway.h" // only for bleevent, where to put it better?

const uint32_t MQTT_MSG_PAYLOAD_SIZE = 200;

class MqttClient : public Resource
{

const int32_t NEW_SUB_SIGNAL      = 0x1;
const int32_t ACK_DONE_SIGNAL     = 0x2;
const int32_t NEW_PUB_SIGNAL      = 0x4;
const int32_t PUBLISH_DONE_SIGNAL = 0x8;
const int32_t SUB_DATA_WRITTEN    = 0x10;

public:
    MqttClient(IPubSub* _proto,
               const std::string& _subtopic,
               const std::string& _pubtopic);
    virtual ~MqttClient() {};
    bool subscribe();
    void subscribtionWritten();
    void publish();

private:
    void publishThread();
    void publishDone(bool status);
    void subscribeThread();
    void subscribeCallback(const uint8_t* data, size_t len);
    void subscribeDone(bool status);
    void ackDone(bool status);

    const std::string subtopic;
    const std::string pubtopic;
    const std::string acktopic;
    rtos::Thread subscriber;
    rtos::Thread publisher;
    std::unique_ptr<uint8_t[]> command;
    volatile bool subscribed;

    char publishContent[MQTT_MSG_PAYLOAD_SIZE];
    char subscribeContent[MQTT_MSG_PAYLOAD_SIZE];

public:
    inline char* getPublishBuffer()
    {
        return publishContent;
    }
};
