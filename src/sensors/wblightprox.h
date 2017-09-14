#pragma once

#include "wunderbarsensor.h"
#include "wunderbarsensordatatypes.h"

class WbLightProx : public WunderbarSensor
{

struct threshold_t
{
    threshold_int16_t white;
    threshold_int16_t proximity;
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

const char* jsonFormat = "{\"light\":%d,\"clr\":{\"r\":%d,\"g\":%d,\"b\":%d},\"prox\":%d}";

public:
    WbLightProx(IBleGateway& _gateway, Resources* _resources);

    virtual const char* getSenseSpec() override;

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    inline int dataToJson(char* outputString, size_t maxLen, const sensor_lightprox_data_t& data)
    {
        return snprintf(outputString,
                        maxLen,
                        jsonFormat,
                        data.white,
                        data.r, data.g, data.b,
                        data.proximity);
    }

private:
    char senseSpec[200];
};
