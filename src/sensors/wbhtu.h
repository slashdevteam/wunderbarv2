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
    threshold_int16_t  temp;
    threshold_int16_t  hum;
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
    WbHtu(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log);
    virtual ~WbHtu() = default;

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    virtual void event(BleEvent _event, const uint8_t* data, size_t len) override;
    virtual void handleCommand(const char* id, const char* data) override;
    virtual CharState sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode) override;
    virtual bool handleWriteUuidRequest(uint16_t uuid, const char* data);

private:
    bool sendConfig(const char* data);
    bool setFrequency(const char* data);
    bool setThreshold(const char* data);
    bool isConfigAllowed(int config);
    size_t configToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data);
    inline size_t dataToJson(char* outputString, size_t maxLen, const uint8_t* data)
    {
        const sensor_htu_data_t& reading = *reinterpret_cast<const sensor_htu_data_t*>(data);
        return std::snprintf(outputString, maxLen, jsonFormat, reading.temperature/100, reading.humidity/100);
    }

private:
    bool defaultRateApplied;
    const uint32_t defaultRate = 1000;
};
