#pragma once

#include "wunderbarsensor.h"

class Bridge : public WunderbarSensor
{
public:
    Bridge(IBleGateway& _gateway, IPubSub* _proto);

private:
    void wunderbarEvent(BleEvent event, uint8_t* data, size_t len);

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

    const char* jsonMqttDataFormatBridgeBegin = "{\"ts\":%ld,\"up_ch_payload\":[";
    const char* jsonMqttDataFormatBridgeEnd   = "]}";

    inline int createJsonDataBridge(char* outputString, size_t maxLen, const sensor_bridge_data_t& data)
    {
        size_t totLen = snprintf(outputString, maxLen, jsonMqttDataFormatBridgeBegin, time(NULL));

        for (auto dataChar = 0; (dataChar < data.payload_length && totLen < maxLen); ++dataChar)
        {   
            // try to write at the end of last write, for the remaining available len
            size_t lenWritten = snprintf(outputString + totLen, maxLen-totLen, "%d,", static_cast<int>(data.payload[dataChar]));

            if (0 < lenWritten)
            {
                totLen += lenWritten;
            }
            else
            {
                //error
                return totLen;
            }
        }

        // remove last coma and finish the string
        size_t lenWritten = snprintf(outputString + totLen - 1, maxLen - totLen + 1, jsonMqttDataFormatBridgeEnd);
        if (0 < lenWritten)
        {
            totLen += lenWritten - 1;
        }

        return totLen;
    }
};
