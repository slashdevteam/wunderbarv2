#include "htu.h"

Htu::Htu(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameHtu),
                        PassKey(defaultPass),
                        mbed::callback(this, &Htu::wunderbarEvent),
                        _proto)
{
};

void Htu::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    //base handling
    WunderbarSensor::wunderbarEvent(event, data, len);

    //htu specific handling
}