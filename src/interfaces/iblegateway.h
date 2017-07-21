#pragma once

#include <list>
#include <deque>
#include <string>
#include <array>
#include <cstring>

#include "Callback.h"

using ServerName = std::string;
using ServerGapAddress = uint8_t[6];
using PassKey = std::array<uint8_t, 8>;

enum class BleEvent
{
    NONE               = 0x0,
    DISCOVERY_COMPLETE = 0x1,
    DISCOVERY_ERROR    = 0x2,
    CONNECTION_OPENED  = 0x3,
    CONNECTION_CLOSED  = 0x4,
    NEW_DATA_READOUT   = 0x5
};


struct ServerIdentificator
{
    ServerGapAddress mac;
    ServerName name;

    ServerIdentificator(const uint8_t* raw)
      : name(reinterpret_cast<const char*>(raw + sizeof(ServerGapAddress)), 14)
    {
        std::memcpy(&mac, raw, sizeof(ServerGapAddress));
    }
};

struct BleServerConfig
{
    ServerName name;
    ServerGapAddress mac;
    PassKey passKey;
};

using BleServerCallback = mbed::Callback<void(BleEvent, const uint8_t*, size_t)>;

class IBleGateway
{
public:
    virtual bool registerServer(BleServerConfig& config, BleServerCallback incomingCallback) = 0;
    virtual void serverDiscoveryComlpete(BleServerConfig& config) = 0;
    virtual bool sendToServer(const BleServerConfig& config, BleServerCallback doneCallback) = 0;
    virtual bool readCharacteristic(const BleServerConfig& server, uint32_t bleCharUuid) = 0;
    virtual bool writeCharacteristic(const BleServerConfig& server,
                                     uint32_t bleCharUuid,
                                     const uint8_t* data,
                                     const size_t len) = 0;
    virtual bool configure() = 0;
    virtual void startOperation() = 0;
    virtual bool storeConfig() = 0;
};
