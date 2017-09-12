#pragma once

#include "ipubsub.h"
#include <unordered_map>

// mbed
#include "PlatformMutex.h"
#include "Thread.h"
#include "Timer.h"
#include "MemoryPool.h"
#include "Queue.h"
#include "Callback.h"

// Paho
#include "MQTTPacket.h"

// defining new struct for MQTT message as the one from MQTTClient is _not_ comaptible with own deserialization
// function (sic!)
struct MqttMessage
{
    int qos;
    uint8_t retained;
    uint8_t dup;
    uint16_t id;
    uint8_t* payload;
    int payloadlen;
};

using MessageTuple = std::tuple<msgTypes, const uint8_t*, const uint8_t*, size_t, MessageDoneCallback, MessageDataCallback>;
using MessageTupleStorage = rtos::MemoryPool<MessageTuple, 16>;
using MessageTupleQueue = rtos::Queue<MessageTuple, 16>;
using Subscribers = std::unordered_map<std::string, MessageDataCallback>;

// 256 bytes are for authToken alone
const int MAX_MQTT_PACKET_SIZE = 500;


class IStdInOut;

struct MqttConfig
{
    char server[40];
    uint32_t port;
    char clientId[30];
    char userId[32];
    char password[256];
} __attribute__ ((__packed__));

enum MQTT_STATUS : int32_t
{
    OK = 0,
    TRANSPORT_LAYER_FAILURE = -1,
    TRANSPORT_LAYER_SEND_FAILURE = -3,
    SERIALIZATION_FAILED = -4,
    CONNACK_NOT_RECEIVED = -5,
    NOT_CONNECTED = -6
};

class MqttProtocol : public IPubSub
{
public:
    MqttProtocol(ITransportLayer* _transport, const MqttConfig& _config, IStdInOut* _log);
    virtual ~MqttProtocol();

    // IPubSub
    virtual bool connect() override;
    virtual bool disconnect() override;
    virtual bool publish(const uint8_t* dest,
                         const uint8_t* data,
                         size_t len,
                         MessageDoneCallback callback) override;
    virtual bool subscribe(const uint8_t* source,
                           MessageDoneCallback doneCallback,
                           MessageDataCallback callback) override;

    // MQTT specific
    void setPingPeriod(int everyMs);

    // make non-copyable C++11 style
    MqttProtocol(const MqttProtocol& other) = delete;
    MqttProtocol& operator=(const MqttProtocol&) = delete;

private:
    void lock();
    void unlock();
    void connecting();
    void dispatch();

    // MQTT specific
    void handleSubscriptions();
    void resetKeepAlive();
    void handleSubscriptionAck();
    void ping();
    void handleMessageQueue();
    void sendPublish(const MQTTString& topic, MessageTuple& message);
    void sendSubscribe(const MQTTString& topic, MessageTuple& message);
    bool sendPacket(size_t length);
    bool receivePacket(msgTypes& msg);
    bool readPacketLength(size_t& len);
    msgTypes receiveTill(msgTypes packetType, int timeout);


private:
    const MqttConfig& config;
    IStdInOut* log;
    PlatformMutex mutex;
    rtos::Thread dispatcher;
    int32_t error;
    int keepAliveHeartbeat;
    mbed::Timer keepAliveTimer;

    MessageTupleStorage messages;
    MessageTupleQueue upstreamQueue;
    Subscribers subscribers;

    // MQTT specific
    uint8_t packetId;
    uint8_t sendbuf[MAX_MQTT_PACKET_SIZE];
    uint8_t readbuf[MAX_MQTT_PACKET_SIZE];
};
