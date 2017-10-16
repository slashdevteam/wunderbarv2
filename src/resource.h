#pragma once

#include <string>
#include <memory>
#include "ipubsub.h"
#include "Thread.h"
#include "MemoryPool.h"
#include "Queue.h"
#include "istdinout.h"

const size_t MQTT_MSG_PAYLOAD_SIZE = 500;

enum MessageType : uint8_t
{
    SUB_REQ = 0,
    PUB_REQ = 1,
    ACK_REQ = 2,
    SUB_INCOMING = 3
};

class IPubSub;
class Resources;

class Resource
{

using ThreadHandle = std::unique_ptr<rtos::Thread>;
using Message = std::array<char, MQTT_MSG_PAYLOAD_SIZE>;
using MessageTuple = std::tuple<MessageType, Message, size_t>;
using MessageTupleStorage = rtos::MemoryPool<MessageTuple, 4>;
using MessageTupleQueue = rtos::Queue<MessageTuple, 4>;
using DataFiller = mbed::Callback<size_t(char*, size_t, const uint8_t*)>;

public:
    Resource(Resources* resources,
             const std::string& _subtopic,
             const std::string& _pubtopic,
             IStdInOut& _log);
    virtual ~Resource();

    virtual void advertise(IPubSub* _proto);
    virtual void revoke();
    virtual size_t getSenseSpec(char* dst, size_t maxLen) = 0;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) = 0;

protected:
    virtual void handleCommand(const char* id, const char* data);
    void publish(DataFiller fillData, const uint8_t* extData);
    void subscribe();
    void acknowledge(const char* commandId, int code);

private:
    void pubSubThread();
    void publishDone(bool status);
    void subscribeCallback(const uint8_t* data, size_t len);
    void subscribeDone(bool status);
    void ackDone(bool status);
    void handleSubscription(MessageTuple& subMsg);
    bool parseSubscription(const char* subscribeContent, std::string& commandID, std::string& data);

private:
    const int32_t ACK_DONE_SIGNAL      = 0x2;
    const int32_t PUBLISH_DONE_SIGNAL  = 0x8;
    const int32_t SUB_DATA_DONE_SIGNAL = 0x10;
    const int32_t SUB_ACK_SIGNAL = 0x20;

    IPubSub* proto;

    MessageTupleStorage msgStorage;
    MessageTupleQueue msgQueue;
    const std::string subtopic;
    const std::string pubtopic;
    ThreadHandle pubSub;
    volatile bool subscribed;
    IStdInOut& log;
};
