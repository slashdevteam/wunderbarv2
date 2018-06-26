#pragma once

struct TlsConfig
{
    uint8_t deviceId[30]; // used for SSL seed
    int authMode;
    const uint8_t* caCert;
    uint8_t* deviceCert;
    uint8_t* key;

} __attribute__ ((__packed__));
