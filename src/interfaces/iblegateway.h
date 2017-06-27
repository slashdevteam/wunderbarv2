#pragma once

#include <list>
#include <string>
#include <array>

#include "Callback.h"

using ServerName = std::string;
using ServerID = uint8_t;

enum class UseMode : uint8_t
{
    NEVER = 0x0,
    ONBOARD = 0x1,
    RUN = 0x2,
    ALWAYS = 0x3
};

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
        return (uuid == other.uuid && parentUuid == other.parentUuid);
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
    uint16_t    parentUuid;
    AccessMode  accessMode;
    uint8_t     dataWriteVerify[16];
} __attribute__((packed));

using RequiredServices = std::list<ServiceDescriptor>;
using Characteristics = std::list<CharcteristicDescriptor>;
using ServerUUID = std::array<uint8_t, 16>;
using PassKey = std::array<uint8_t, 8>;
using Security = bool;

struct BleServerConfig
{
    ServerName name;
    ServerID id;
    RequiredServices requiredServices;
    Characteristics characteristics;
    ServerUUID uuid;
    PassKey passKey;
    Security security;
};

using BleServerCallback = mbed::Callback<void(const uint8_t* data, size_t len)>;

class IBleGateway
{
public:
    virtual bool registerServer(BleServerConfig& config, BleServerCallback incomingCallback) = 0;
    virtual bool sendToServer(const BleServerConfig& config, BleServerCallback doneCallback) = 0;
    virtual bool configure() = 0;
    virtual bool storeConfig() = 0;
};
