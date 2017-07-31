
#pragma once

#include "wunderbarsensor.h"

class Gyro : public WunderbarSensor
{
public:
    Gyro(IBleGateway& _gateway, IPubSub* _proto);

private:
    void wunderbarEvent(BleEvent event, uint8_t* data, size_t len);

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

    const char* jsonMqttDataFormatGyro = "{\"ts\":%ld,\"gyro\":{\"x\":%05ld.00,\"y\":%05ld.00,\"z\":%05ld.00},\"accel\":{\"x\":%05d.00,\"y\":%05d.00,\"z\":%05d.00}}";

    inline int createJsonDataGyro(char* outputString, size_t maxLen, const sensor_gyro_data_t& data)
    {
        return snprintf(outputString, maxLen, jsonMqttDataFormatGyro, time(NULL),
                        data.gyro.x/100, data.gyro.y/100, data.gyro.z/100,
                        data.acc.x/100, data.acc.y/100, data.acc.z/100);
    }   

};