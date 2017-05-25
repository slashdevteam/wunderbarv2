#pragma once

#include <cstdint>

#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

class NetworkInterface;

namespace mbed
{
    class Stream;
}

struct ProtocolConfig
{
    char server[40];
    uint32_t port;
};

class Protocol
{
public:
    Protocol(NetworkInterface* _network, const ProtocolConfig& _config, mbed::Stream* _stdout);
    ~Protocol();

    bool connect() { return true;};

public:
    const char name[4];

private:
    NetworkInterface* net;
    ProtocolConfig config;
    mbed::Stream* log;

    // SSL
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbg;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config sslConf;
    mbedtls_ssl_session savedSession;
};
