#include "gyro.h"

Gyro::Gyro(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameGyro),
                        PassKey(defaultPass),
                        mbed::callback(this, &Gyro::wunderbarEvent),
                        _proto)
{
};

void Gyro::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    //base handling
    WunderbarSensor::wunderbarEvent(event, data, len);

    //htu specific handling
}