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

enum class AccessMode : uint8_t
{
    NONE  = 0x0,
    READ  = 0x1,
    WRITE = 0x2,
    RW    = 0x3
};

struct CharcteristicDescriptor
{
    uint16_t    uuid;
    AccessMode  mode;

    CharcteristicDescriptor(uint16_t _uuid, AccessMode  _mode)
        : uuid(_uuid),
          mode(_mode)
    {
    }

    bool operator==(const CharcteristicDescriptor& other) const
    {
        return (uuid == other.uuid);
    }

    void* data()
    {
        return &uuid;
    }

    const void* data() const
    {
        return &uuid;
    }
} __attribute__((packed));

using BleServerCallback = mbed::Callback<void(BleEvent, const uint8_t*, size_t)>;

class IBleGateway
{
public:
    virtual bool registerServer(BleServerConfig& config, BleServerCallback incomingCallback) = 0;
    virtual void serverDiscoveryComlpete(BleServerConfig& config) = 0;
    virtual bool sendToServer(const BleServerConfig& config, BleServerCallback doneCallback) = 0;
    virtual bool requestCharacteristicRead(const BleServerConfig& server, uint16_t bleCharUuid) = 0;
    virtual bool requestCharacteristicWrite(const BleServerConfig& server,
                                            uint16_t bleCharUuid,
                                            const uint8_t* data,
                                            const size_t len) = 0;
    virtual bool configure() = 0;
    virtual void startOperation() = 0;
    virtual bool storeConfig() = 0;
};
