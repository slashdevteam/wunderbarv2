#pragma once

#include "itransportlayer.h"

#include <memory>

// mbed
#include "TCPSocket.h"

// SSL
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

class NetworkStack;
namespace mbed
{
    class Stream;
}

struct TlsConfig
{
    uint8_t deviceId[30]; // used for SSL seed @TODO: use random seed
    uint8_t caCert[1425];
    uint8_t deviceCert[1514];
    uint8_t key[3244];

} __attribute__ ((__packed__));

class TLS : public ITransportLayer
{
public:
    TLS(NetworkStack* _network, const TlsConfig& _config, mbed::Stream* _log);
    virtual ~TLS();

    virtual bool connect(const char* server, size_t port) override;
    virtual bool disconnect() override;
    virtual size_t send(const uint8_t* data, size_t len) override;
    virtual size_t receive(uint8_t* data, size_t len) override;

private:
    void connecting();

    // SSL
    int32_t sslInit();
    int32_t sslSeed();
    int32_t sslConfigDefaults();
    int32_t sslParseCrt(mbedtls_x509_crt& crt, const uint8_t* crtBuffer, size_t crtLen);
    int32_t sslParseKey();
    int32_t sslSetup();
    int32_t sslHandshake();



private:
    NetworkStack* network;
    TCPSocket socket;
    const TlsConfig& config;
    mbed::Stream* log;
    std::shared_ptr<const char> server;
    size_t port;

    // SSL specific
    int32_t error;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config sslConf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctrDrbg;
    mbedtls_x509_crt caCert;
    mbedtls_x509_crt deviceCert;
    mbedtls_pk_context pkey;
};
