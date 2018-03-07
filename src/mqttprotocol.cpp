#include "mqttprotocol.h"
#include "itransportlayer.h"
#include "istdinout.h"

#include <memory>
#include <tuple>

// mbed
#include "Timer.h"
#include "mbed_wait_api.h"

// MQTT
#include "MQTTConnect.h"

const int COMMAND_TIMEOUT = 5000;

const char* MESSAGE_NAMES[] =
{
    "NONE",
    "CONNECT",
    "CONNACK",
    "PUBLISH",
    "PUBACK",
    "PUBREC",
    "PUBREL",
    "PUBCOMP",
    "SUBSCRIBE",
    "SUBACK",
    "UNSUBSCRIBE",
    "UNSUBACK",
    "PINGREQ",
    "PINGRESP",
    "DISCONNECT",
    "UNKNOWN"
};

MqttProtocol::MqttProtocol(ITransportLayer* _transport, const MqttConfig& _config, IStdInOut* _log)
    : IPubSub(_transport, "MQTT"),
      config(_config),
      log(_log),
      dispatcher(osPriorityNormal, 4096, nullptr, "MQTT-DISPATCH"),
      error(MQTT_STATUS::NOT_CONNECTED),
      pingSent(false),
      keepAliveHeartbeat(10000),
      packetId(0)
{
}

MqttProtocol::~MqttProtocol()
{
    disconnect();
}

bool MqttProtocol::isConnected()
{
    return MQTT_STATUS::OK == error;
}

void MqttProtocol::loop()
{
    if(0 == error)
    {
        // ready to start busy polling on tcpSocket
        dispatcher.start(mbed::callback(this, &MqttProtocol::dispatch));
        dispatcher.join();
    }
}

bool MqttProtocol::connect()
{
    rtos::Thread connector(osPriorityNormal, 0x1000);
    connector.start(mbed::callback(this, &MqttProtocol::connecting));
    connector.join();
    log->printf("\r\nConnection status %d\r\n", error);
    if(0 != error)
    {
        transport->disconnect();
    }
    return (0 == error);
}

bool MqttProtocol::disconnect()
{
    if(MQTT_STATUS::OK == error)
    {
        transport->disconnect();
    }

    return true; //@TODO: add graceful MQTT DISCONNECT handling
}

bool MqttProtocol::publish(const uint8_t* dest,
                           const uint8_t* data,
                           size_t len,
                           mbed::Callback<void(bool)> callback)
{
    bool ret = false;

    MessageTuple* message = messages.alloc();
    if(message)
    {
        *message = std::make_tuple(PUBLISH, dest, data, len, callback, mbed::Callback<void(const uint8_t*, size_t)>());
        osStatus status = upstreamQueue.put(message);
        if(osOK == status)
        {
            ret = true;
        }
    }

    return ret;
}

bool MqttProtocol::subscribe(const uint8_t* source,
                             MessageDoneCallback doneCallback,
                             MessageDataCallback callback)
{
    bool ret = false;

    MessageTuple* message = messages.alloc();
    if(message)
    {
        *message = std::make_tuple(SUBSCRIBE, source, nullptr, 0, doneCallback, callback);
        osStatus status = upstreamQueue.put(message);
        if(osOK == status)
        {
            ret = true;
        }
    }

    return ret;
}

void MqttProtocol::setPingPeriod(int everyMs)
{
    lock();
    keepAliveHeartbeat = everyMs;
    unlock();
}

void MqttProtocol::lock()
{
    mutex.lock();
}

void MqttProtocol::unlock()
{
    mutex.unlock();
}

void MqttProtocol::connecting()
{
    error = MQTT_STATUS::TRANSPORT_LAYER_FAILURE;

    transport->setTimeout(5000);
    if(transport->connect(config.server, config.port))
    {
        MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
        connectData.MQTTVersion = 3;
        connectData.clientID.cstring = config.clientId;
        connectData.username.cstring = config.userId;
        connectData.password.cstring = config.password;

        int packetLen = MQTTSerialize_connect(sendbuf, MAX_MQTT_PACKET_SIZE, &connectData);
        if(packetLen <= 0)
        {
            error = MQTT_STATUS::SERIALIZATION_FAILED;
            log->printf("MQTT connect msg sent fail - %d\r\n", error);
            return;
        }

        if(!sendPacket(static_cast<size_t>(packetLen)))
        {
            error = MQTT_STATUS::TRANSPORT_LAYER_SEND_FAILURE;
            log->printf("Error sending the connect request packet\r\n");
            return;
        }

        msgTypes msg = static_cast<msgTypes>(receiveTill(CONNACK, COMMAND_TIMEOUT));

        if(CONNACK != msg)
        {
            error = MQTT_STATUS::CONNACK_NOT_RECEIVED;
            log->printf("MQTT CONNACK failed\r\n");
            return;
        }

        error = MQTT_STATUS::OK;
    }
}

void MqttProtocol::dispatch()
{
    keepAliveTimer.start();
    while(1)
    {
        // first and foremost try to read packet
        msgTypes msg = static_cast<msgTypes>(DISCONNECT + 1); // DISCONNECT is last in msgTypes enum
        transport->setTimeout(50);
        if(receivePacket(msg))
        {
            log->printf("%s msg type: %s\r\n", __PRETTY_FUNCTION__, MESSAGE_NAMES[msg]);

            switch(msg)
            {
                case PUBLISH:
                    handleSubscriptions();
                    break;
                case CONNACK: // should only come in connector thread
                    break;
                case PUBACK: // QoS 1, 2
                    break;
                case SUBACK: // processed synchronously in handleMessageQueue
                    break;
                case PINGRESP:
                    pingSent = false;
                    log->printf("Ping OK\r\n");
                    break;
                default:
                    log->printf("%s Unknown message type = %d\r\n", __PRETTY_FUNCTION__, msg);
                    break;
            }
        }
        // process message queue
        handleMessageQueue();

        if(keepAliveTimer.read_ms() > keepAliveHeartbeat)
        {
            ping();
        }

        if(!isConnected())
        {
            break;
        }
    }
}

void MqttProtocol::handleSubscriptions()
{
    // check if anyone is actually listening
    if(!subscribers.empty())
    {
        MQTTString mqttTopic = MQTTString_initializer;
        MqttMessage msg;
        int32_t ret = MQTTDeserialize_publish(&msg.dup,
                                              &msg.qos,
                                              &msg.retained,
                                              &msg.id,
                                              &mqttTopic,
                                              &msg.payload,
                                              &msg.payloadlen,
                                              readbuf,
                                              MAX_MQTT_PACKET_SIZE);
        if(1 == ret)
        {
            std::string topic;
            // special handling needed for odd MQTTString type
            if(mqttTopic.lenstring.len > 0)
            {
                topic = std::string((const char*)mqttTopic.lenstring.data, (size_t)mqttTopic.lenstring.len);
            }
            else
            {
                topic = (const char*)mqttTopic.cstring;
            }

            auto subscriber = subscribers.find(topic);
            if(subscriber != subscribers.end())
            {
                subscriber->second(msg.payload, msg.payloadlen);
            }
        }
    }
}

void MqttProtocol::resetKeepAlive()
{
    keepAliveTimer.reset();
    keepAliveTimer.start();
}

void MqttProtocol::ping()
{
    if(!pingSent)
    {
        pingSent = true;
        size_t len = MQTTSerialize_pingreq(sendbuf, MAX_MQTT_PACKET_SIZE);

        if((len > 0) && sendPacket(len))
        {
            log->printf("Ping sent\r\n");
            resetKeepAlive();
        }
        else
        {
            log->printf("Ping sending FAILED!\r\n");
            handleError();
        }
    }
    else
    {
        log->printf("Ping response not received!\r\n");
        handleError();
    }
}

void MqttProtocol::handleMessageQueue()
{
    osEvent msg = upstreamQueue.get(10);

    if(osEventMessage == msg.status)
    {
        MessageTuple& message = *reinterpret_cast<MessageTuple*>(msg.value.p);
        const char* messageTopic = reinterpret_cast<const char*>(std::get<1>(message));
        const size_t messageTopicLen = std::strlen(messageTopic);
        // full topic is userId + / clientId + / + message topic + \0
        size_t fullTopicLen = std::strlen(config.userId)
                             + 1
                             + std::strlen(config.clientId)
                             + 1
                             + messageTopicLen
                             + 1;
        std::unique_ptr<char []> fullTopic = std::make_unique<char []>(fullTopicLen);
        std::memset(fullTopic.get(), 0, fullTopicLen);
        std::snprintf(fullTopic.get(), fullTopicLen, "%s/%s/%s", config.userId, config.clientId, messageTopic);
        MQTTString mqttTopic = MQTTString_initializer;
        mqttTopic.cstring = fullTopic.get();

        transport->setTimeout(5000);
        log->printf("Sending message to topic: %s\r\n", fullTopic.get());
        bool messageOk = false;
        switch(std::get<msgTypes>(message))
        {
            case PUBLISH:
                messageOk = sendPublish(mqttTopic, message);
                break;
            case SUBSCRIBE:
                messageOk = sendSubscribe(mqttTopic, message);
                break;
            default:
                break;
        }
        messages.free(&message);
        if(!messageOk)
        {
            handleError();
        }
        log->printf("%s\r\n", messageOk ? "OK" : "ERROR");
    }
}

bool MqttProtocol::sendPublish(const MQTTString& topic, MessageTuple& message)
{
    bool ret = true;

    size_t packetLen = MQTTSerialize_publish(sendbuf,
                                             MAX_MQTT_PACKET_SIZE,
                                             0,
                                             0, // QoS
                                             false,
                                             packetId++,
                                             topic,
                                             std::get<2>(message),
                                             std::get<3>(message));
    log->printf("Message: %s\r\n", std::get<2>(message));
    if((packetLen <= 0) || (!sendPacket(packetLen)))
    {
        log->printf("Publish to topic: %s failed!\r\n", topic.cstring);
        ret = false;
    }
    std::get<4>(message)(ret);
    return ret;
}

bool MqttProtocol::sendSubscribe(const MQTTString& topic, MessageTuple& message)
{
    bool ret = true;
    int qos = 0;
    size_t len = MQTTSerialize_subscribe(sendbuf,
                                         MAX_MQTT_PACKET_SIZE,
                                         0,
                                         packetId++,
                                         1,
                                         &topic,
                                         &qos);

    if((len <= 0) || (!sendPacket(len)))
    {
        log->printf("Subscribe packet to topic: %s failed!\r\n", topic.cstring);
        ret = false;
    }

    msgTypes msg = receiveTill(SUBACK, 3000);

    if(SUBACK != msg)
    {
        log->printf("MQTT SUBACK failed!\r\n");
        ret = false;
    }

    uint16_t subackid;
    int subQos = -1;
    int count = -1;
    int32_t status = MQTTDeserialize_suback(&subackid, 1, &count, &subQos, readbuf, MAX_MQTT_PACKET_SIZE);

    if((1 != status) || (subQos == -1) || (subQos == 0x80))
    {
        log->printf("MQTT SUBACK wrong QoS = %d!\r\n", subQos);
        ret = false;
    }

    // SUBACK received ok,
    // so add callback for topic
    subscribers.emplace(std::make_pair(topic.cstring, std::get<5>(message)));

    std::get<4>(message)(ret);
    return ret;
}

void MqttProtocol::handleError()
{
    // GS1500M reports an error when it's already too late
    // so we reconnect again on new socket
    transport->disconnect();
    log->printf("Reconnecting to MQTT!\r\n");
    connecting();

    // if this fails, MqttProtocol::loop will end and WiFi connection will be reestablished
}

bool MqttProtocol::sendPacket(size_t length)
{
    size_t part = 0;
    size_t sent = 0;

    while (sent < length)
    {
        part = transport->send(sendbuf, length);
        if(part == 0)
        {
            break;
        }
        sent += part;
    }

    return (sent == length);
}

bool MqttProtocol::receivePacket(msgTypes& msg)
{
    MQTTHeader header = {0};
    size_t len = 0;
    size_t packetLength = 0;
    size_t received = 0;

    /* 1. read the header byte.  This has the packet type in it */
    received = transport->receive(readbuf, 1);
    if(received == 0)
    {
        return false;
    }

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    if(!readPacketLength(packetLength))
    {
        return false;
    }

    len += MQTTPacket_encode(readbuf + 1, packetLength); /* put the original remaining length into the buffer */

    if (packetLength > (MAX_MQTT_PACKET_SIZE - len))
    {
        return false;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if(packetLength >0)
    {
        received = transport->receive(readbuf + len, packetLength);
        if(received != packetLength)
        {
            return false;
        }
    }

    // Convert the header to type
    header.byte = readbuf[0];
    msg = static_cast<msgTypes>(header.bits.type);

    return true;
}

bool MqttProtocol::readPacketLength(size_t& len)
{
    uint8_t c;
    int multiplier = 1;
    size_t controlLen = 0;
    size_t received = 0;
    const size_t MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    len = 0;
    do
    {
        if(++controlLen > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            return false;
        }

        received = transport->receive(&c, 1);
        if(received == 0)
        {
            return false;
        }

        len += (c & 127) * multiplier;
        multiplier *= 128;
    }
    while((c & 128) != 0);

    return true;
}

msgTypes MqttProtocol::receiveTill(msgTypes packetType, int timeout)
{
    msgTypes msg = static_cast<msgTypes>(DISCONNECT + 1); // DISCONNECT is last in msgTypes enum
    mbed::Timer timer;
    timer.start();

    do
    {
        if(!receivePacket(msg))
        {
            break;
        }

        if (timer.read_ms() > timeout)
        {
            break;
        }

    }
    while(msg != packetType);

    return msg;
}
