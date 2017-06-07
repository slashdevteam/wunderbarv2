#include "protocol.h"
#include "Stream.h"
#include "mbedtls/entropy_poll.h"
#include "MQTTPacket.h" // cause MQTTConnect does not include MQTTString!
#include "Timer.h"
#include <cstring> // strlen
#include "error.h"

using mbed::Timer;
using rtos::Thread;

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

const int32_t COMMAND_TIMEOUT = 5000;

// free function because of C style interface
int pollEntropy(void *,
                unsigned char *output,
                size_t len,
                size_t *olen)
{
    uint32_t rdm = 0;

    rdm = time(nullptr);

    for(uint16_t i = 0; i < len; ++i)
    {
        srand(rdm);
        output[i] = rand() % 256;
    }
    *olen = len;

    return 0;
}

const int32_t DEFAULT_SOCKET_TIMEOUT = 1000;
/**
 * Receive callback for mbed TLS
 */
static int ssl_recv(void *ctx, unsigned char *buf, size_t len)
{
    int recv = -1;
    TCPSocket *socket = static_cast<TCPSocket *>(ctx);
    socket->set_timeout(DEFAULT_SOCKET_TIMEOUT);
    recv = socket->recv(buf, len);

    if (NSAPI_ERROR_WOULD_BLOCK == recv) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    } else if (recv < 0) {
        return -1;
    } else {
        return recv;
    }
}

/**
 * Send callback for mbed TLS
 */
static int ssl_send(void *ctx, const unsigned char *buf, size_t len)
{
    int sent = -1;
    TCPSocket *socket = static_cast<TCPSocket *>(ctx);
    socket->set_timeout(DEFAULT_SOCKET_TIMEOUT);
    sent = socket->send(buf, len);

    if(NSAPI_ERROR_WOULD_BLOCK == sent) {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    } else if (sent < 0) {
        return -1;
    } else {
        return sent;
    }
}

Protocol::Protocol(NetworkStack* _network, const ProtocolConfig& _config, mbed::Stream* log)
    : name{'M', 'Q', 'T', 'T', 0},
      net(_network),
      config(_config),
      log(log),
      tcpSocket(net),
      listener(osPriorityNormal, 8192)
{
    // Used to randomize source port
    unsigned int seed;
    size_t len;
    mbedtls_hardware_poll(NULL, (unsigned char *) &seed, sizeof seed, &len);
    srand(seed);

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&sslConf);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_x509_crt_init(&devicecert);
    mbedtls_pk_init(&pkey);
    mbedtls_ctr_drbg_init(&ctrDrbg);
    mbedtls_entropy_init(&entropy);

    mbedtls_entropy_add_source(&entropy,
                               pollEntropy,
                               NULL,
                               128,
                               0); // weak entropy source @TODO: check for strong source?

}

Protocol::~Protocol()
{
    mbedtls_ssl_config_free(&sslConf);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_free(&ssl);
}

bool Protocol::connect()
{
    // Connection uses _a lot_ of stack for AES/CRT
    Thread connector(osPriorityNormal, 0x4000);
    connector.start(mbed::callback(this, &Protocol::connectThread));
    connector.join();
    log->printf("Connection status %d\n", error);
    return (0 == error);
}

bool Protocol::disconnect()
{
    pinger.terminate();
    listener.terminate();
    tcpSocket.close();
    int32_t ret = mbedtls_ssl_session_reset(&ssl);
    return (0 == ret);
}

bool Protocol::publish(const std::string& _topic, const std::string& _data, uint8_t _id)
{
    bool ret = true;
    lock();
    size_t topicLen = _topic.size();
    char* topic = new char[topicLen + 1]; // +1 for NULL
    std::memcpy(topic, _topic.c_str(), topicLen);
    topic[topicLen] = '\0';

    MQTTString mqttTopic = MQTTString_initializer;
    mqttTopic.cstring = topic;

    size_t len = MQTTSerialize_publish(sendbuf,
                                       MAX_MQTT_PACKET_SIZE,
                                       0,
                                       0, // QoS
                                       false,
                                       _id,
                                       mqttTopic,
                                       const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(_data.c_str())), // Paho is not const correct!
                                       (int) _data.size());

    if((len <= 0) || (sendPacket(len) != 0))
    {
        log->printf("Publish to topic: %s failed!\r\n", _topic);
        ret = false;
    }
    delete topic;
    unlock();
    return ret;
}

bool Protocol::subscribe(const std::string& _topic, mbed::Callback<void(const char*)> func, uint8_t _id)
{
    lock();

    int32_t qos = 0;

    size_t topicLen = _topic.size();
    char* topic = new char[topicLen + 1]; // +1 for NULL
    std::memcpy(topic, _topic.c_str(), topicLen);
    topic[topicLen] = '\0';

    MQTTString mqttTopic = MQTTString_initializer;
    mqttTopic.cstring = topic;

    size_t len = MQTTSerialize_subscribe(sendbuf,
                                  MAX_MQTT_PACKET_SIZE,
                                  0,
                                  _id,
                                  1,
                                  &mqttTopic,
                                  (int*)&qos);

    if((len <= 0) || (sendPacket(len) != 0))
    {
        log->printf("Subscribe packet to topic: %s failed!\r\n", _topic);
        return false;
    }

    msgTypes msg = static_cast<msgTypes>(readUntil(SUBACK, COMMAND_TIMEOUT));

    if(SUBACK != msg)
    {
        log->printf("MQTT SUBACK failed!\r\n");
        return false;
    }

    uint16_t subackid;
    int subQos = -1;
    int count = -1;
    int32_t ret = MQTTDeserialize_suback(&subackid, 1, &count, &subQos, readbuf, MAX_MQTT_PACKET_SIZE);

    if((1 != ret) || (subQos == -1) || (subQos == 0x80))
    {
        log->printf("MQTT SUBACK wrong QoS = %d!\r\n", subQos);
        return false;
    }

    // SUBACK received ok,
    // so add callback for topic
    subscribers.emplace(std::make_pair(_topic, func));
    // and start listening thread on 1st subscription
    if(rtos::Thread::State::Running != listener.get_state())
    {
        listener.start(mbed::callback(this, &Protocol::listenerThread));
    }

    unlock();
    return false;
}

void Protocol::keepAlive(size_t everyMs)
{
    keepAliveHeartbeat = everyMs;
    if(rtos::Thread::State::Running != pinger.get_state())
    {
        pinger.start(mbed::callback(this, &Protocol::keepAliveThread));
    }
}

void Protocol::keepAliveThread()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    while(1)
    {
        rtos::Thread::wait(keepAliveHeartbeat);
        lock();
        log->printf("%s lock\r\n", __PRETTY_FUNCTION__);
        size_t len = MQTTSerialize_pingreq(sendbuf, MAX_MQTT_PACKET_SIZE);
        if (len > 0 && (sendPacket(len) == 0))
        {
            log->printf("Ping OK\r\n");
        }

        msgTypes msg = static_cast<msgTypes>(readUntil(PINGRESP, 5*COMMAND_TIMEOUT));
        if(PINGRESP != msg)
        {
            log->printf("PINGRESP not received - %d!\r\n", msg);
        }
        log->printf("%s unlock\r\n", __PRETTY_FUNCTION__);
        unlock();
    }
}

void Protocol::listenerThread()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    while(1)
    {
        rtos::Thread::wait(LISTENER_PERIOD_MS);
        lock();
        log->printf("%s lock\r\n", __PRETTY_FUNCTION__);
        // check if anyone is actually listening
        if(!subscribers.empty())
        {
            log->printf("%s read\r\n", __PRETTY_FUNCTION__);
            msgTypes msgType = static_cast<msgTypes>(readPacket());
            log->printf("%s read = %d\r\n", __PRETTY_FUNCTION__, msgType);
            if(PUBLISH == msgType)
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
                        subscriber->second(reinterpret_cast<char*>(msg.payload));
                    }
                }
            }
        }
        log->printf("%s unlock\r\n", __PRETTY_FUNCTION__);
        unlock();
    }
}

void Protocol::lock()
{
    mutex.lock();
}

void Protocol::unlock()
{
    mutex.unlock();
}

void Protocol::connectThread()
{
    error = sslInit();
    if(error)
    {
        log->printf("SSL Init fail - %d\r\n", error);
        return;
    }

    error = mbedtls_ssl_session_reset(&ssl);
    if(error)
    {
        log->printf("SSL reset session fail - %d\r\n", error);
        return;
    }

    error = mbedtls_ssl_set_hostname(&ssl, config.server);
    if(error)
    {
        log->printf("SSL Set hostname fail - %d\r\n", error);
        return;
    }

    mbedtls_ssl_set_bio(&ssl,
                        static_cast<void*>(&tcpSocket),
                        ssl_send,
                        ssl_recv,
                        nullptr);
    // tcpSocket.set_timeout(DEFAULT_SOCKET_TIMEOUT);
    error = tcpSocket.connect(config.server, config.port);
    if(error)
    {
        log->printf("TCP connect fail - %d\r\n", error);
        tcpSocket.close();
        return;
    }

    // mbed API lacks Socket event checking (callback is void() and [TCP]Socket does not expose
    // interface for getting socket state/event), so busy polling/listening is needed for MQTT subscriptions :(
    // socketMonitor.start(mbed::callback(this, &Protocol::socketSignalThread));
    // tcpSocket.sigio(mbed::callback(this, &Protocol::socketSignal));

    error = sslHandshake();
    if(error)
    {
        log->printf("SSL handshake fail - %d\r\n", error);
        tcpSocket.close();
        return;
    }

    error = mqttConnect();
    if(error)
    {
        log->printf("MQTT Connect fail - %d\r\n", error);
        tcpSocket.close();
        return;
    }
}

int32_t Protocol::mqttConnect()
{
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    // need to copy client credentials because Paho is _not_ const correct
    char* clientId = new char[std::strlen(config.clientId)];
    std::memcpy(clientId, config.clientId, std::strlen(config.clientId));
    char* username = new char[std::strlen(config.userId)];
    std::memcpy(username, config.userId, std::strlen(config.userId));
    char* password = new char[std::strlen(config.password)];
    std::memcpy(password, config.password, std::strlen(config.password));
    connectData.MQTTVersion = 3;
    connectData.clientID.cstring = clientId;
    connectData.username.cstring = username;
    connectData.password.cstring = password;

    size_t sentLen = MQTTSerialize_connect(sendbuf, MAX_MQTT_PACKET_SIZE, &connectData);
    int32_t error = (sentLen <= 0);
    if(error)
    {
        log->printf("MQTT connect msg sent fail - %d\r\n", error);
        return error;
    }

    error = sendPacket((size_t)sentLen);
    if(error)  // send the connect packet
    {
        log->printf("Error sending the connect request packet %d\r\n", error);
        return error;
    }

    msgTypes msg = static_cast<msgTypes>(readUntil(CONNACK, COMMAND_TIMEOUT));

    if(CONNACK != msg)
    {
        log->printf("MQTT CONNACK fail - %d\r\n", error);
        return -4;
    }

    delete clientId;
    delete username;
    delete password;

    return 0;
}

int32_t Protocol::readBytesToBuffer(char * buffer, size_t size, int32_t timeout)
{
    int rc;
    // Timer timer;
    // timer.start();
    // log->printf("%d\r\n", timer.read_ms());
    // while((timer.read_ms() < timeout) && (rc != MBEDTLS_ERR_SSL_WANT_READ))
    // {
        rc = mbedtls_ssl_read(&ssl, (uint8_t*)buffer, size);
        if (MBEDTLS_ERR_SSL_WANT_READ == rc)
            return -2;
        else
            return rc;
        // log->printf("%d\r\n", timer.read_ms());
    // }
    // log->printf("%d\r\n", timer.read_ms());
    return rc;
}

int32_t Protocol::readPacketLength(int32_t* value)
{
    int rc = MQTTPACKET_READ_ERROR;
    unsigned char c;
    int multiplier = 1;
    int len = 0;
    const int MAX_NO_OF_REMAINING_LENGTH_BYTES = 4;

    *value = 0;
    do
    {
        if (++len > MAX_NO_OF_REMAINING_LENGTH_BYTES)
        {
            rc = MQTTPACKET_READ_ERROR; /* bad data */
            goto exit;
        }

        rc = readBytesToBuffer((char *) &c, 1, DEFAULT_SOCKET_TIMEOUT);
        if (rc != 1)
        {
            rc = MQTTPACKET_READ_ERROR;
            goto exit;
        }

        *value += (c & 127) * multiplier;
        multiplier *= 128;
    } while ((c & 128) != 0);

    rc = MQTTPACKET_READ_COMPLETE;

exit:
    if (rc == MQTTPACKET_READ_ERROR )
        len = -1;

    return len;
}

int32_t Protocol::readPacket()
{
    int32_t rc = -1;
    MQTTHeader header = {0};
    int32_t len = 0;
    int32_t rem_len = 0;

    /* 1. read the header byte.  This has the packet type in it */
    if ( (rc = readBytesToBuffer((char*)&readbuf[0], 1, DEFAULT_SOCKET_TIMEOUT)) != 1)
        goto exit;

    len = 1;
    /* 2. read the remaining length.  This is variable in itself */
    if (readPacketLength(&rem_len) < 0 )
        goto exit;

    len += MQTTPacket_encode(readbuf + 1, rem_len); /* put the original remaining length into the buffer */

    if (rem_len > (MAX_MQTT_PACKET_SIZE - len))
    {
        rc = -3;
        goto exit;
    }

    /* 3. read the rest of the buffer using a callback to supply the rest of the data */
    if (rem_len > 0 && (readBytesToBuffer((char *) (readbuf + len), rem_len, DEFAULT_SOCKET_TIMEOUT) != rem_len))
        goto exit;

    // Convert the header to type
    // and update rc
    header.byte = readbuf[0];
    rc = header.bits.type;

exit:

    return rc;
}

int32_t Protocol::sendPacket(size_t length)
{
    int rc = -1;
    size_t sent = 0;

    while (sent < length)
    {
        rc = sendBytesFromBuffer((char *) &sendbuf[sent], length - sent, DEFAULT_SOCKET_TIMEOUT);
        if (rc < 0)  // there was an error writing the data
            break;
        sent += rc;
    }

    if (sent == length)
        rc = 0;
    else
        rc = -1;

    return rc;
}

int32_t Protocol::sendBytesFromBuffer(char * buffer, size_t size, int timeout)
{
    int rc;

    // Do SSL/TLS write
    rc =  mbedtls_ssl_write(&ssl, (const unsigned char *) buffer, size);
    if (MBEDTLS_ERR_SSL_WANT_WRITE == rc)
        return -2;
    else
        return rc;
}

int32_t Protocol::readUntil(int32_t packetType, int32_t timeout)
{
    int pType = -1;
    Timer timer;

    timer.start();
    do {
        pType = readPacket();
        if (pType < 0)
            break;

        if (timer.read_ms() > timeout)
        {
            pType = -1;
            break;
        }
    }while(pType != packetType);

    return pType;
}

int32_t Protocol::sslHandshake()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    int32_t sslError = mbedtls_ssl_handshake(&ssl);
    if(sslError)
    {
        char strerror[300];
        mbedtls_strerror(sslError, strerror, 300);
        log->printf("TLS Handshake fail - %d %s\r\n", sslError, strerror);
        return sslError;
    }

    sslError = mbedtls_ssl_get_verify_result(&ssl);
    log->printf("SSL verify result - %d\r\n", sslError);

    return sslError;
}

int32_t Protocol::sslInit()
{
    int32_t sslError = sslSeed();
    if(sslError)
    {
        log->printf("SSL Seed fail - %d\r\n", sslError);
        return sslError;
    }

    sslError = sslConfigDefaults();
    if(sslError)
    {
        log->printf("SSL Defaults fail - %d\r\n", sslError);
        return sslError;
    }

    sslError = sslParseCrt(cacert, config.cacert, std::strlen(reinterpret_cast<const char*>(&config.cacert[0])));
    if(sslError)
    {
        log->printf("SSL CA CRT fail - %d\r\n", sslError);
        return sslError;
    }

    sslError = sslParseCrt(devicecert, config.devicecert, std::strlen(reinterpret_cast<const char*>(config.devicecert)));
    if(sslError)
    {
        log->printf("SSL CRT fail - %d\r\n", sslError);
        return sslError;
    }

    sslError = sslParseKey();
    if(sslError)
    {
        log->printf("SSL PKEY fail - %d\r\n", sslError);
        return sslError;
    }

    mbedtls_ssl_conf_own_cert(&sslConf, &devicecert, &pkey);
    mbedtls_ssl_conf_authmode(&sslConf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&sslConf, &cacert, nullptr);
    mbedtls_ssl_conf_rng(&sslConf, mbedtls_ctr_drbg_random, &ctrDrbg);

    sslError = sslSetup();
    if(sslError)
    {
        log->printf("SSL fail - %d\r\n", sslError);
        return sslError;
    }

    return sslError;
}

int32_t Protocol::sslSetup()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_ssl_setup(&ssl,
                             &sslConf);
}

int32_t Protocol::sslParseKey()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_pk_parse_key(&pkey,
                                config.key,
                                std::strlen(reinterpret_cast<const char*>(config.key)) + 1,
                                nullptr,
                                0);
}

int32_t Protocol::sslParseCrt(mbedtls_x509_crt& crt, const uint8_t* crtBuffer, size_t crtLen)
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_x509_crt_parse(&crt,
                                  crtBuffer,
                                  crtLen + 1); // +1 for null character
}

int32_t Protocol::sslConfigDefaults()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_ssl_config_defaults(&sslConf,
                                       MBEDTLS_SSL_IS_CLIENT,
                                       MBEDTLS_SSL_TRANSPORT_STREAM,
                                       0);
}

int32_t Protocol::sslSeed()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_ctr_drbg_seed(&ctrDrbg,
                                 mbedtls_entropy_func,
                                 &entropy,
                                 config.deviceId,
                                 std::strlen(reinterpret_cast<const char*>(config.deviceId)));
}


