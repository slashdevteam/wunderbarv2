#include "tls.h"
#include "istdinout.h"
#include <cstring>

#include <memory>
// mbed
#include "Thread.h"

// SSL
#include "mbedtls/entropy_poll.h"
#include "mbedtls/debug.h"
#include "error.h"
#include "pollentropy.h"

const int32_t DEFAULT_SOCKET_TIMEOUT = 1000;

#include "nouartfix.h"
static void my_debug(void *ctx, int level, const char *file, int line,
                         const char *str)
{
    const char *p, *basename;
    (void) ctx;

    // Extract basename from file
    for(p = basename = file; *p != '\0'; p++) {
        if(*p == '/' || *p == '\\') {
            basename = p + 1;
        }
    }

    cdcPrintfRetarget("%s:%04d: |%d| %s", basename, line, level, str);
}

static int sslRecv(void* ctx, unsigned char* buf, size_t len)
{
    int recv = -1;
    TCPSocket* socket = static_cast<TCPSocket*>(ctx);
    socket->set_timeout(DEFAULT_SOCKET_TIMEOUT);
    recv = socket->recv(buf, len);

    if (NSAPI_ERROR_WOULD_BLOCK == recv)
    {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }
    else if (recv < 0)
    {
        return -1;
    }
    else
    {
        return recv;
    }
}

/**
 * Send callback for mbed TLS
 */
static int sslSend(void* ctx, const unsigned char* buf, size_t len)
{
    int sent = -1;
    TCPSocket* socket = static_cast<TCPSocket*>(ctx);
    socket->set_timeout(DEFAULT_SOCKET_TIMEOUT);
    sent = socket->send(buf, len);

    if(NSAPI_ERROR_WOULD_BLOCK == sent)
    {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    }
    else if (sent < 0)
    {
        return -1;
    }
    else
    {
        return sent;
    }
}

TLS::TLS(NetworkStack* _network, const TlsConfig& _config, IStdInOut* _log)
    : network(_network),
      socket(network),
      config(_config),
      log(_log)
{
    log->printf("%s\n", __PRETTY_FUNCTION__);
    // Used to randomize source port
    unsigned int seed;
    size_t len;
    mbedtls_hardware_poll(NULL, (unsigned char *) &seed, sizeof seed, &len);
    srand(seed);

    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&sslConf);
    mbedtls_x509_crt_init(&caCert);
    if(config.deviceCert)
    {
        mbedtls_x509_crt_init(&deviceCert);
    }
    if(config.key)
    {
        mbedtls_pk_init(&pkey);
    }
    mbedtls_ctr_drbg_init(&ctrDrbg);
    mbedtls_entropy_init(&entropy);

    mbedtls_entropy_add_source(&entropy,
                               pollEntropy,
                               NULL,
                               128,
                               0); // weak entropy source @TODO: check for strong source?
    // calling sslInit here, because mbed TLS teardown does not check for nullptrs before
    // memset (and buffers are null after mbedtls_ssl_init!)
    error = sslInit();
    if(error)
    {
        log->printf("SSL Init fail - %d\r\n", error);
    }

    mbedtls_ssl_conf_dbg(&sslConf, my_debug, NULL);
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
}

TLS::~TLS()
{
    log->printf("%s\n", __PRETTY_FUNCTION__);
    disconnect();
    mbedtls_ssl_config_free(&sslConf);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctrDrbg);
    if(config.deviceCert)
    {
        mbedtls_x509_crt_free(&deviceCert);
    }
    if(config.key)
    {
        mbedtls_pk_free(&pkey);
    }
    mbedtls_x509_crt_free(&caCert);
    mbedtls_ssl_free(&ssl);
}

bool TLS::connect(const char* _server, size_t _port)
{
    // attempt to connect only if sslInit was ok
    if(error == 0)
    {
        // Connection uses _a lot_ of stack for AES/CRT
        rtos::Thread connector(osPriorityNormal, 0x2000);
        // copy parameters to members as mbed callbacks do not allow type erasure as std::bind
        std::strncpy(server, _server, sizeof(server));
        port = _port;
        connector.start(mbed::callback(this, &TLS::connecting));
        connector.join();
        log->printf("Connection status %d\n", error);
    }

    return (0 == error);
}

bool TLS::disconnect()
{
    socket.close();
    int32_t ret = mbedtls_ssl_session_reset(&ssl);
    return (0 == ret);
}

size_t TLS::send(const uint8_t* data, size_t len)
{
    int sent = 0;

    sent = mbedtls_ssl_write(&ssl, data, len);
    if(sent < 0)
    {
        error = sent;
        sent = 0;
    }

    return sent;
}

size_t TLS::receive(uint8_t* data, size_t len)
{
    int received = 0;

    received = mbedtls_ssl_read(&ssl, data, len);
    if(received < 0)
    {
        return 0;
    }

    return received;
}

void TLS::connecting()
{
    error = mbedtls_ssl_session_reset(&ssl);
    if(error)
    {
        log->printf("SSL reset session fail - %d\r\n", error);
        return;
    }

    error = mbedtls_ssl_set_hostname(&ssl, server);
    if(error)
    {
        log->printf("SSL Set hostname fail - %d\r\n", error);
        return;
    }

    mbedtls_ssl_set_bio(&ssl,
                        static_cast<void*>(&socket),
                        sslSend,
                        sslRecv,
                        nullptr);

    error = socket.connect(server, port);
    if(error)
    {
        log->printf("TCP connect fail - %d\r\n", error);
        socket.close();
        return;
    }

    // mbed API lacks Socket event checking (callback is void() and [TCP]Socket does not expose
    // interface for getting socket state/event), so busy polling/listening is needed for MQTT subscriptions :(
    // socketMonitor.start(mbed::callback(this, &TLS::socketSignalThread));
    // socket.sigio(mbed::callback(this, &TLS::socketSignal));

    error = sslHandshake();
    if(error)
    {
        log->printf("SSL handshake fail - %d\r\n", error);
        socket.close();
        return;
    }
}

int32_t TLS::sslHandshake()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    int32_t sslError = mbedtls_ssl_handshake(&ssl);
    if(sslError)
    {
        char strerror[300];
        mbedtls_strerror(sslError, strerror, 300);
        log->printf("TLS Handshake fail - %x %s\r\n", sslError, strerror);
        return sslError;
    }

    sslError = mbedtls_ssl_get_verify_result(&ssl);
    log->printf("SSL verify result - %d\r\n", sslError);

    return sslError;
}

int32_t TLS::sslInit()
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

    sslError = sslParseCrt(caCert, config.caCert, std::strlen(reinterpret_cast<const char*>(config.caCert)));
    if(sslError)
    {
        log->printf("SSL CA CRT fail - %d\r\n", sslError);
        return sslError;
    }

    if(config.deviceCert)
    {
        sslError = sslParseCrt(deviceCert, config.deviceCert, std::strlen(reinterpret_cast<const char*>(config.deviceCert)));
        if(sslError)
        {
            log->printf("SSL CRT fail - %d\r\n", sslError);
            return sslError;
        }
    }

    if(config.key)
    {
        sslError = sslParseKey();
        if(sslError)
        {
            log->printf("SSL PKEY fail - %d\r\n", sslError);
            return sslError;
        }
    }

    mbedtls_ssl_conf_own_cert(&sslConf, &deviceCert, &pkey);
    mbedtls_ssl_conf_authmode(&sslConf, config.authMode);
    mbedtls_ssl_conf_ca_chain(&sslConf, &caCert, nullptr);
    mbedtls_ssl_conf_rng(&sslConf, mbedtls_ctr_drbg_random, &ctrDrbg);

    sslError = sslSetup();
    if(sslError)
    {
        log->printf("SSL fail - %d\r\n", sslError);
        return sslError;
    }

    return sslError;
}

int32_t TLS::sslSetup()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_ssl_setup(&ssl,
                             &sslConf);
}

int32_t TLS::sslParseKey()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_pk_parse_key(&pkey,
                                config.key,
                                std::strlen(reinterpret_cast<const char*>(config.key)) + 1,
                                nullptr,
                                0);
}

int32_t TLS::sslParseCrt(mbedtls_x509_crt& crt, const uint8_t* crtBuffer, size_t crtLen)
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_x509_crt_parse(&crt,
                                  crtBuffer,
                                  crtLen + 1); // +1 for null character
}

int32_t TLS::sslConfigDefaults()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_ssl_config_defaults(&sslConf,
                                       MBEDTLS_SSL_IS_CLIENT,
                                       MBEDTLS_SSL_TRANSPORT_STREAM,
                                       0);
}

int32_t TLS::sslSeed()
{
    log->printf("%s\r\n", __PRETTY_FUNCTION__);
    return mbedtls_ctr_drbg_seed(&ctrDrbg,
                                 mbedtls_entropy_func,
                                 &entropy,
                                 config.deviceId,
                                 std::strlen(reinterpret_cast<const char*>(config.deviceId)));
}
