#pragma once

#include <cstdint>

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "TCPSocket.h"
#include "Callback.h"
#include "PlatformMutex.h"
#include "Thread.h"

class NetworkInterface;

const int32_t MAX_MQTT_PACKET_SIZE = 200;

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
public:
    Protocol(NetworkInterface* _network, const ProtocolConfig& _config, mbed::Stream* log);
    ~Protocol();

    bool connect();
    bool disconnect();

    bool publish(const char* _data);
    bool subscribe(const char* _data, mbed::Callback<void()> func);
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

    void lock();
    void unlock();

private:
    NetworkInterface* net;
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
    // mbedtls_ssl_session savedSession;

    // MQTT
    TCPSocket tcpSocket;
    uint8_t sendbuf[MAX_MQTT_PACKET_SIZE];
    uint8_t readbuf[MAX_MQTT_PACKET_SIZE];
    rtos::Thread pinger;
    size_t keepAliveHeartbeat;
};
