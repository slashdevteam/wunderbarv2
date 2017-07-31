#pragma once

#include "wunderbarsensor.h"

class WbHtu : public WunderbarSensor
{
public:
    WbHtu(IBleGateway& _gateway, IPubSub* _proto);

private:
    void wunderbarEvent(BleEvent event, const uint8_t* data, size_t len);

    struct sensor_htu_data_t
    {
        int16_t temperature;
        int16_t humidity;
    } __attribute__((packed));

    struct threshold_t
    {
        threshold_int16_t  temperature;
        threshold_int16_t  humidity;
    } __attribute__((packed));

    enum sensor_htu_config_t
    {
        HTU21D_RH_12_TEMP14 = 0,
        HTU21D_RH_8_TEMP12,
        HTU21D_RH_10_TEMP13,
        HTU21D_RH_11_TEMP11,
    } __attribute__((packed));

    const char* jsonMqttDataFormatHtu = "{\"ts\":%ld,\"temp\":%05d.00,\"hum\":%05d.00}";

    inline int dataToJson(char* outputString, size_t maxLen, const sensor_htu_data_t& data)
    {
        return snprintf(outputString, maxLen, jsonMqttDataFormatHtu, time(NULL), data.temperature/100, data.humidity/100);
    };
};
