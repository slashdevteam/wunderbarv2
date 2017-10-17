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
    WbBridge(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log);
    virtual ~WbBridge() = default;

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    virtual void handleCommand(const char* id, const char* data) override;
    virtual CharState sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode) override;
    virtual bool handleWriteUuidRequest(uint16_t uuid, const char* data);

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    size_t configToJson(char* outputString, size_t maxLen, const uint8_t* data);
    bool isBaudrateAllowed(int baudRate);
    bool setState(const char* data);
    bool sendConfig(const char* data);
private:
    sensor_bridge_data_t dataDown;
    uint8_t relayState;
};
