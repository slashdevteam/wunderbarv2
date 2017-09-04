#pragma once

#include <string>
#include <memory>
#include "ipubsub.h"
#include "Thread.h"

const uint32_t MQTT_MSG_PAYLOAD_SIZE = 200;

class IPubSub;
class Resources;

class Resource
{
public:
    Resource(Resources* resources,
             const std::string& _subtopic,
             const std::string& _pubtopic);
    virtual ~Resource() {};

    virtual void advertise(IPubSub* _proto);
    bool subscribe();
    void publish();
    bool acknowledge(const std::string& _command,
                     int _code,
                     MessageDoneCallback doneCallback);
    void writeDone();
    virtual const char* getSenseSpec() = 0;
    virtual const char* getActuateSpec() = 0;

protected:
    virtual int handleCommand(const char* command);


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
    bool parseSubscription(std::string& commandID, std::string& data);

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
    rtos::Thread subscriber;
    rtos::Thread publisher;
    volatile bool subscribed;
};
