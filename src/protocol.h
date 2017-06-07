#pragma once

#include <cstdint>

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "TCPSocket.h"
#include "Callback.h"
#include "PlatformMutex.h"
#include "Thread.h"
#include <string>
#include <memory>
#include <unordered_map>

class NetworkStack;

const int32_t MAX_MQTT_PACKET_SIZE = 200;
using Subscribers = std::unordered_map<std::string, mbed::Callback<void(const char*)>>;
namespace mbed
{
    class Stream;
}

struct ProtocolConfig
{
    char server[40];
    uint32_t port;
    uint8_t deviceId[30]; // used for SSL seed @TODO: use random seed
    uint8_t cacert[1425];
    uint8_t devicecert[1514];
    uint8_t key[3244];
    char clientId[32];
    char userId[32];
    char password[32];

};

class Protocol
{
const int32_t LISTENER_PERIOD_MS = 30000;

public:
    Protocol(NetworkStack* _network, const ProtocolConfig& _config, mbed::Stream* log);
    ~Protocol();

    bool connect();
    bool disconnect();

    bool publish(const std::string& _topic, const std::string& _data, uint8_t _id);
    bool subscribe(const std::string& _topic, mbed::Callback<void(const char*)> func, uint8_t _id);
    void keepAlive(size_t everyMs);

public:
    const char name[5];


private:
    int32_t sslInit();
    int32_t sslSeed();
    int32_t sslConfigDefaults();
    int32_t sslParseCrt(mbedtls_x509_crt& crt, const uint8_t* crtBuffer, size_t crtLen);
    int32_t sslParseKey();
    int32_t sslSetup();
    int32_t sslHandshake();

    int32_t mqttConnect();
    int32_t readUntil(int32_t packetType, int32_t timeout);
    int32_t readPacket();
    int32_t readPacketLength(int32_t* value);
    int32_t readBytesToBuffer(char * buffer, size_t size, int32_t timeout);
    int32_t sendPacket(size_t length);
    int32_t sendBytesFromBuffer(char * buffer, size_t size, int timeout);

    void connectThread();
    void keepAliveThread();
    void listenerThread();

    void lock();
    void unlock();

private:
    NetworkStack* net;
    const ProtocolConfig& config;
    mbed::Stream* log;
    PlatformMutex mutex;

    // SSL
    int32_t error;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config sslConf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbg;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt devicecert;
    mbedtls_pk_context pkey;

    // MQTT
    TCPSocket tcpSocket;
    uint8_t sendbuf[MAX_MQTT_PACKET_SIZE];
    uint8_t readbuf[MAX_MQTT_PACKET_SIZE];
    rtos::Thread pinger;
    rtos::Thread listener;
    size_t keepAliveHeartbeat;
    Subscribers subscribers;
};
