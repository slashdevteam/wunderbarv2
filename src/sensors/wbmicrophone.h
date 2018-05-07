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

const char* jsonFormat = "\"level\":%d";

public:
    WbMicrophone(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log);
    virtual ~WbMicrophone() = default;

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    virtual void event(BleEvent _event, const uint8_t* data, size_t len) override;
    virtual void handleCommand(const char* id, const char* data) override;
    virtual CharState sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode) override;
    virtual bool handleWriteUuidRequest(uint16_t uuid, const char* data);

private:
    bool setFrequency(const char* data);
    bool setThreshold(const char* data);
    size_t thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data);
    inline size_t dataToJson(char* outputString, size_t maxLen, const uint8_t* data)
    {
        const sensor_microphone_data_t& reading = *reinterpret_cast<const sensor_microphone_data_t*>(data);
        return std::snprintf(outputString, maxLen, jsonFormat, reading.mic_level);
    }

private:
    bool defaultRateApplied;
    const uint32_t defaultRate = 10000;
};
