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

    virtual const char* getSenseSpec() override;
    virtual const char* getActuateSpec() override;

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    int dataToJson(char* outputString, size_t maxLen, const sensor_bridge_data_t& data);

private:
    char senseSpec[200];
    char actuateSpec[200];
};
