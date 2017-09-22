#pragma once

#include "wunderbarsensor.h"
#include "wunderbarsensordatatypes.h"

class WbGyro : public WunderbarSensor
{

struct threshold_t
{
    threshold_int32_t gyro;
    threshold_int16_t acc;
} __attribute__((packed));

enum gyroFullScale_t
{
    GYRO_FULL_SCALE_250DPS = 0,
    GYRO_FULL_SCALE_500DPS,
    GYRO_FULL_SCALE_1000DPS,
    GYRO_FULL_SCALE_2000DPS
};

enum accFullScale_t
{
    ACC_FULL_SCALE_2G = 0,
    ACC_FULL_SCALE_4G,
    ACC_FULL_SCALE_8G,
    ACC_FULL_SCALE_16G
};

struct config_t
{
    gyroFullScale_t gyro_full_scale;
    accFullScale_t  acc_full_scale;
} __attribute__((packed));

struct gyroCoordinates_t
{
    int32_t x;
    int32_t y;
    int32_t z;
} __attribute__((packed));

struct accCoordinates_t
{
    int16_t x;
    int16_t y;
    int16_t z;
} __attribute__((packed));

struct sensor_gyro_data_t
{
    gyroCoordinates_t gyro;
    accCoordinates_t  acc;
} __attribute__((packed));

const char* jsonFormat = "\"gyro\":{\"x\":%5ld,\"y\":%5ld,\"z\":%5ld},\"accel\":{\"x\":%5d,\"y\":%5d,\"z\":%5d}";

public:
    WbGyro(IBleGateway& _gateway, Resources* _resources);

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    inline int dataToJson(char* outputString, size_t maxLen, const sensor_gyro_data_t& data)
    {
        return snprintf(outputString,
                        maxLen,
                        jsonFormat,
                        data.gyro.x/100, data.gyro.y/100, data.gyro.z/100,
                        data.acc.x/100, data.acc.y/100, data.acc.z/100);
    }
};
