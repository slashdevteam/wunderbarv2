#pragma once

#include "wunderbarsensor.h"
#include "wunderbarsensordatatypes.h"

class WbLightProx : public WunderbarSensor
{

struct threshold_t
{
    threshold_int16_t white;
    threshold_int16_t prox;
} __attribute__((packed));

enum rgbc_gain_t
{
    RGBC_GAIN_1     = 0x00,
    RGBC_GAIN_4     = 0x01,
    RGBC_GAIN_16    = 0x02,
    RGBC_GAIN_60    = 0x03
};

enum prox_drive_t
{
    PROX_DRIVE_12_5_MA = 0xC0,
    PROX_DRIVE_25_MA   = 0x80,
    PROX_DRIVE_50_MA   = 0x40,
    PROX_DRIVE_100_MA  = 0x00
};

struct sensor_lightprox_config_t
{
    rgbc_gain_t  rgbc_gain;
    prox_drive_t prox_drive;
} __attribute__((packed));

struct sensor_lightprox_data_t
{
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t white;
    uint16_t proximity;
} __attribute__((packed));

const char* jsonFormat = "\"white\":%d,\"red\":%d,\"green\":%d,\"blue\":%d,\"proximity\":%d";

public:
    WbLightProx(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log);
    virtual ~WbLightProx() = default;

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
    bool isRgbcGainAllowed(int scale);
    bool isProxDriveAllowed(int scale);
    size_t configToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data);
    inline size_t dataToJson(char* outputString, size_t maxLen, const uint8_t* data)
    {
        const sensor_lightprox_data_t& reading = *reinterpret_cast<const sensor_lightprox_data_t*>(data);
        return std::snprintf(outputString,
                             maxLen,
                             jsonFormat,
                             reading.white,
                             reading.r,
                             reading.g,
                             reading.b,
                             reading.proximity);
    }

private:
    bool defaultRateApplied;
    const uint32_t defaultRate = 1000;
};
