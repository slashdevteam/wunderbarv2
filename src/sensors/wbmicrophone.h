#pragma once

#include "wunderbarsensor.h"

class WbMicrophone : public WunderbarSensor
{
public:
    WbMicrophone(IBleGateway& _gateway, IPubSub* _proto);

private:
    void wunderbarEvent(BleEvent event, const uint8_t* data, size_t len);

    struct threshold_t
    {
        threshold_int16_t mic_level;
    } __attribute__((packed));

    struct sensor_microphone_data_t
    {
        int16_t mic_level;
    } __attribute__((packed));

    const char* jsonMqttDataFormatMic = "{\"ts\":%ld,\"snd_level\":%d}";

    inline int dataToJson(char* outputString, size_t maxLen, const sensor_microphone_data_t& data)
    {
        return snprintf(outputString, maxLen, jsonMqttDataFormatMic, time(NULL), data.mic_level);
    }
    
};
