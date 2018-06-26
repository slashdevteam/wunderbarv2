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

const char* jsonFormat = "\"gyro\":[%ld,%ld,%ld],\"accel\":[%d,%d,%d]";

public:
    WbGyro(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log);
    virtual ~WbGyro() = default;

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
    bool isGyroScaleAllowed(int scale);
    bool isAccScaleAllowed(int scale);
    size_t configToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data);
    inline size_t dataToJson(char* outputString, size_t maxLen, const uint8_t* data)
    {
        const sensor_gyro_data_t& reading = *reinterpret_cast<const sensor_gyro_data_t*>(data);
        return std::snprintf(outputString,
                             maxLen,
                             jsonFormat,
                             reading.gyro.x/100, reading.gyro.y/100, reading.gyro.z/100,
                             reading.acc.x/100, reading.acc.y/100, reading.acc.z/100);
    }

private:
    bool defaultRateApplied;
    const uint32_t defaultRate = 1000;
};
