#pragma once

#include <cstdint>
#include "nsapi_types.h"
#include "tls.h"
#include "mqttprotocol.h"

struct RestConfig
{
    char server[40];
    uint32_t port;
    char token[100];
    char path[100];
    TlsConfig tls;
} __attribute__ ((__packed__));

namespace wunderbar
{

struct WiFiConfig
{
    char ssid[32];
    char pass[64];
    nsapi_security_t security;
    uint8_t channel;
} __attribute__ ((__packed__));

struct Configuration
{
    WiFiConfig wifi;
    TlsConfig tls;
    MqttConfig proto;
    RestConfig rest;
} __attribute__ ((__packed__));

}
