#pragma once

#include "wunderbarsensor.h"
#include "wunderbarsensordatatypes.h"

class WbMicrophone : public WunderbarSensor
{

struct threshold_t
{
    threshold_int16_t mic_level;
} __attribute__((packed));

struct sensor_microphone_data_t
{
    int16_t mic_level;
} __attribute__((packed));

const char* jsonFormat = "\"snd_level\":%d";

public:
    WbMicrophone(IBleGateway& _gateway, Resources* _resources);
    virtual ~WbMicrophone() = default;

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    inline size_t dataToJson(char* outputString, size_t maxLen, const uint8_t* data)
    {
        const sensor_microphone_data_t& reading = *reinterpret_cast<const sensor_microphone_data_t*>(data);
        return std::snprintf(outputString, maxLen, jsonFormat, reading.mic_level);
    }
};
