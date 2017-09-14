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

const char* jsonFormat = "{\"snd_level\":%d}";

public:
    WbMicrophone(IBleGateway& _gateway, Resources* _resources);

    virtual const char* getSenseSpec() override;

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    inline int dataToJson(char* outputString, size_t maxLen, const sensor_microphone_data_t& data)
    {
        return snprintf(outputString, maxLen, jsonFormat, data.mic_level);
    }

private:
    char senseSpec[200];
};
