#pragma once

#include "wunderbarsensor.h"

class WbBridge : public WunderbarSensor
{

constexpr static uint32_t BRIDGE_PAYLOAD_SIZE = 19;
const uint32_t BRIDGE_HEDER_SIZE   = 2;
const uint32_t BRIDGE_CRC16_SIZE   = 2;
const uint32_t BRIDGE_PACKET_SIZE  = BRIDGE_PAYLOAD_SIZE + BRIDGE_HEDER_SIZE + BRIDGE_CRC16_SIZE;

struct sensor_bridge_data_t
{
    uint8_t payload_length;
    uint8_t payload[BRIDGE_PAYLOAD_SIZE];
} __attribute__((packed));

struct config_t
{
    uint32_t baud_rate;
} __attribute__((packed));

public:
    WbBridge(IBleGateway& _gateway, Resources* _resources);
    virtual ~WbBridge() = default;

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;
    virtual void advertise(IPubSub* _proto) override;

protected:
    virtual int handleCommand(const char* command) override;
    bool parseCommand(const char* data);

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);

private:
    sensor_bridge_data_t dataDown;
    uint8_t relayState;
};
