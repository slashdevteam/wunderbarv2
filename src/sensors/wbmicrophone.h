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

const char* jsonFormat = "{\"ts\":%ld,\"snd_level\":%d}";

public:
    WbMicrophone(IBleGateway& _gateway, Resources* _resources);

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    inline int dataToJson(char* outputString, size_t maxLen, const sensor_microphone_data_t& data)
    {
        return snprintf(outputString, maxLen, jsonFormat, time(NULL), data.mic_level);
    }

};
