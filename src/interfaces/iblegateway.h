#pragma once

#include <list>
#include <deque>
#include <string>
#include <array>
#include <cstring>

#include "Callback.h"

using ServerName = std::string;
using ServerGapAddress = uint8_t[6];
using ServerHandle = uint8_t;

enum class AccessMode : uint8_t
{
    NONE = 0x0,
    READ = 0x1,
    WRITE = 0x2,
    VERIFY = 0x4,
    RW = 0x3
};

struct CharcteristicDescriptor
{
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

    uint16_t    uuid;
    AccessMode  mode;
} __attribute__((packed));

enum class UseMode : uint8_t
{
    NEVER = 0x0,
    ONBOARD = 0x1,
    RUN = 0x2,
    ALWAYS = 0x3
};

inline bool operator&(const UseMode& a, const UseMode& b)
{
    return ((static_cast<uint8_t>(a)) & (static_cast<uint8_t>(b)));
}

using Characteristics = std::deque<CharcteristicDescriptor>;

struct ServiceDescriptor
{
    bool operator==(const ServiceDescriptor& other) const
    {
        return (uuid == other.uuid && type == other.type);
    }

    void* data()
    {
        return &uuid;
    }

    const void* data() const
    {
        return &uuid;
    }

    uint16_t    uuid;
    uint8_t     type;
    UseMode     useMode;

} __attribute__((packed));

struct BleService
{
    bool operator==(const BleService& other) const
    {
        return (descriptor == other.descriptor);
    }

    ServiceDescriptor descriptor;
    Characteristics characteristics;
};

enum class BleEvent
{
    NONE               = 0x0,
    DISCOVERY_COMPLETE = 0x1,
    DISCOVERY_ERROR    = 0x2,
    CONNECTION_OPENED  = 0x3,
    CONNECTION_CLOSED  = 0x4,
    NEW_DATA_READOUT   = 0x5
};

using RequiredServices = std::deque<BleService>;
using ServerUUID = std::array<uint8_t, 16>;
using PassKey = std::array<uint8_t, 8>;

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
    ServerHandle handle;
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
    virtual bool readCharacteristic(const BleServerConfig& server, const CharcteristicDescriptor& characteristic) = 0;
    virtual bool writeCharacteristic(const BleServerConfig& server,
                                     const CharcteristicDescriptor& characteristic,
                                     const uint8_t* data,
                                     const size_t len) = 0;
    virtual bool configure() = 0;
    virtual bool storeConfig() = 0;
};
