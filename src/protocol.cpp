#include "protocol.h"
#include "Stream.h"

Protocol::Protocol(NetworkInterface* _network, const ProtocolConfig& _config, mbed::Stream* _stdout)
    : name{'M', 'Q', 'T', 'T'},
      net(_network),
      config(_config),
      log(_stdout)
{
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctrDrbg);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&sslConf);
    memset( &savedSession, 0, sizeof( mbedtls_ssl_session ) );
}

Protocol::~Protocol()
{
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    mbedtls_x509_crt_free(&cacert);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&sslConf);
}
