#pragma once

#include "wunderbarsensor.h"
#include "wunderbarsensordatatypes.h"

class WbHtu : public WunderbarSensor
{

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

const char* jsonFormat = "\"temp\":%d,\"hum\":%d";

public:
    WbHtu(IBleGateway& _gateway, Resources* _resources);
    virtual ~WbHtu() = default;

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    inline size_t dataToJson(char* outputString, size_t maxLen, const uint8_t* data)
    {
        const sensor_htu_data_t& reading = *reinterpret_cast<const sensor_htu_data_t*>(data);
        return std::snprintf(outputString, maxLen, jsonFormat, reading.temperature/100, reading.humidity/100);
    };
};
