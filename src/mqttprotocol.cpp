#include "mqttprotocol.h"
#include "itransportlayer.h"
#include "istdinout.h"

#include <memory>
#include <tuple>

// mbed
#include "Timer.h"

// MQTT
#include "MQTTConnect.h"

const int COMMAND_TIMEOUT = 5000;

MqttProtocol::MqttProtocol(ITransportLayer* _transport, const MqttConfig& _config, IStdInOut* _log)
    : IPubSub(_transport, "MQTT"),
      config(_config),
      log(_log),
      dispatcher(osPriorityNormal, 8192),
      error(MQTT_STATUS::OK),
      keepAliveHeartbeat(10000),
      packetId(0)
{
}

MqttProtocol::~MqttProtocol()
{
    disconnect();
}

bool MqttProtocol::connect()
{
    rtos::Thread connector(osPriorityNormal, 2048);
    connector.start(mbed::callback(this, &MqttProtocol::connecting));
    connector.join();
    log->printf("Connection status %d\n", error);
    if(0 == error)
    {
        // ready to start busy polling on tcpSocket
        dispatcher.start(mbed::callback(this, &MqttProtocol::dispatch));
    }
    else
    {
        transport->disconnect();
    }
    return (0 == error);
}

bool MqttProtocol::disconnect()
{
    dispatcher.terminate();
    transport->disconnect();
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

    if(transport->connect(config.server, config.port))
    {
        MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
        // need to copy client credentials because Paho is _not_ const correct
        std::unique_ptr<char[]> clientId = std::make_unique<char[]>(std::strlen(config.clientId));
        std::memcpy(clientId.get(), config.clientId, std::strlen(config.clientId));
        std::unique_ptr<char[]> username = std::make_unique<char[]>(std::strlen(config.userId));
        std::memcpy(username.get(), config.userId, std::strlen(config.userId));
        std::unique_ptr<char[]> password = std::make_unique<char[]>(std::strlen(config.password));
        std::memcpy(password.get(), config.password, std::strlen(config.password));

        connectData.MQTTVersion = 3;
        connectData.clientID.cstring = clientId.get();
        connectData.username.cstring = username.get();
        connectData.password.cstring = password.get();

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
        if(receivePacket(msg))
        {
            log->printf("%s msg %d\r\n", __PRETTY_FUNCTION__, msg);

            switch(msg)
            {
                case PUBLISH:
                    handleSubscriptions();
                    break;
                case PINGRESP:
                    resetKeepAlive();
                    break;
                case CONNACK: // should only come in connector thread
                    break;
                case PUBACK: // QoS 1, 2
                    break;
                case SUBACK: // processed synchronously in handleMessageQueue
                    break;
                default:
                    log->printf("%s Unknown message type = %d\r\n", __PRETTY_FUNCTION__, msg);
                    break;
            }
        }

        if(keepAliveTimer.read_ms() > keepAliveHeartbeat)
        {
            ping();
        }

        // process message queue
        handleMessageQueue();
    }
}

void MqttProtocol::handleSubscriptions()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
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

void MqttProtocol::handleSubscriptionAck()
{

}

void MqttProtocol::ping()
{
    size_t len = MQTTSerialize_pingreq(sendbuf, MAX_MQTT_PACKET_SIZE);
    if (len > 0 && (sendPacket(len) == 0))
    {
        log->printf("Ping OK\r\n");
    }
}

void MqttProtocol::handleMessageQueue()
{
    osEvent msg = upstreamQueue.get(10);
    if(osEventMessage == msg.status)
    {
        MessageTuple& message = *reinterpret_cast<MessageTuple*>(msg.value.p);
        // prepare topic
        const char* _topic = reinterpret_cast<const char*>(std::get<1>(message));
        size_t topicLen = std::strlen(_topic) + 1; // +1 for NULL
        std::unique_ptr<char[]> topic = std::make_unique<char[]>(topicLen);
        std::memcpy(topic.get(), _topic, topicLen);
        topic[topicLen] = '\0';

        MQTTString mqttTopic = MQTTString_initializer;
        mqttTopic.cstring = topic.get();

        switch(std::get<msgTypes>(message))
        {
            case PUBLISH:
                sendPublish(mqttTopic, message);
                break;
            case SUBSCRIBE:
                sendSubscribe(mqttTopic, message);
                break;
            default:
                break;
        }
        messages.free(&message);
    }
}

void MqttProtocol::sendPublish(const MQTTString& topic, MessageTuple& message)
{
    bool ret = true;
    uint8_t* data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(std::get<2>(message))); // Paho is not const correct!
    int len = static_cast<int>(std::get<3>(message)); // Paho mixes length and error codes, so length is signed (sic!)

    size_t packetLen = MQTTSerialize_publish(sendbuf,
                                       MAX_MQTT_PACKET_SIZE,
                                       0,
                                       0, // QoS
                                       false,
                                       packetId++,
                                       *const_cast<MQTTString*>(&topic), // Paho and consts correctness :/
                                       data,
                                       len);
    if((packetLen <= 0) || (!sendPacket(packetLen)))
    {
        log->printf("Publish to topic: %s failed!\r\n", topic.cstring);
        ret = false;
    }
    std::get<4>(message)(ret);
}

void MqttProtocol::sendSubscribe(const MQTTString& topic, MessageTuple& message)
{
    bool ret = true;
    int qos = 0;
    size_t len = MQTTSerialize_subscribe(sendbuf,
                                         MAX_MQTT_PACKET_SIZE,
                                         0,
                                         packetId++,
                                         1,
                                         const_cast<MQTTString*>(&topic), // Paho and consts correctness :/
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
