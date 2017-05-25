#pragma once

#include <cstdint>
#include "nsapi_types.h"
#include "protocol.h"

namespace wunderbar
{

struct WiFiConfig
{
    char ssid[32];
    char pass[64];
    nsapi_security_t security;
    uint8_t channel;
};

struct Configuration
{
    WiFiConfig wifi;
    ProtocolConfig proto;
};

}
