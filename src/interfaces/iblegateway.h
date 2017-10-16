#pragma once

#include <list>
#include <deque>
#include <string>
#include <array>
#include <cstring>

#include "Callback.h"

constexpr size_t PASS_KEY_LEN = 8;
using ServerName = std::string;
using ServerGapAddress = uint8_t[6];
using PassKey = std::array<uint8_t, PASS_KEY_LEN>;


enum class BleEvent
{
    NONE               = 0x0,
    DISCOVERY_COMPLETE = 0x1,
    DISCOVERY_ERROR    = 0x2,
    CONNECTION_OPENED  = 0x3,
    CONNECTION_CLOSED  = 0x4,
    WRITE_OK           = 0x5,

    DATA_SENSOR_ID                  = 0x10,
    DATA_SENSOR_BEACON_FREQUENCY    = 0x11,
    DATA_SENSOR_FREQUENCY           = 0x12,
    DATA_SENSOR_THRESHOLD           = 0x13,
    DATA_SENSOR_CONFIG              = 0x14,
    DATA_SENSOR_NEW_DATA            = 0x15,
    DATA_BATTERY_LEVEL              = 0x16,
    DATA_MANUFACTURER_NAME          = 0x17,
    DATA_HARDWARE_REVISION          = 0x18,
    DATA_FIRMWARE_REVISION          = 0x19
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

enum class SensorAvailability : uint8_t
{
    NOT_CHECKED  = 0x0,
    AVAILABLE  = 0x1,
    NOT_AVAILABLE = 0x2,
    IGNORE = 0x3
};

struct BleConfig
{
    SensorAvailability sensorAvailability[6];
} __attribute__ ((__packed__));

using BleServerCallback = mbed::Callback<void(BleEvent, const uint8_t*, size_t)>;

class IBleGateway
{
public:
    IBleGateway() = default;
    virtual ~IBleGateway() = default;

    virtual bool registerServer(BleServerConfig& config, BleServerCallback incomingCallback) = 0;
    virtual void serverDiscoveryComlpete(BleServerConfig& config) = 0;
    virtual bool requestRead(const BleServerConfig& server, uint16_t bleCharUuid) = 0;
    virtual bool requestWrite(const BleServerConfig& server,
                              uint16_t bleCharUuid,
                              const uint8_t* data,
                              const size_t len) = 0;
    virtual bool configure(const BleConfig& bleConfig) = 0;
    virtual bool onboard(BleConfig& config) = 0;
    virtual void setTimeout(uint32_t timeout) = 0;
    virtual void startOperation() = 0;
    virtual void stopOperation() = 0;
    virtual bool storeConfig() = 0;

    // make non-copyable C++11 style
    IBleGateway(const IBleGateway& other) = delete;
    IBleGateway& operator=(const IBleGateway&) = delete;
};
