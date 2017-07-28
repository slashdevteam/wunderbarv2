#include "infrared.h"

InfraRed::InfraRed(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(sensorNameInfraRed),
                      PassKey(defaultPass),
                      mbed::callback(this, &InfraRed::wunderbarEvent),
                      _proto)
{
};

void InfraRed::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    //base handling
    WunderbarSensor::wunderbarEvent(event, data, len);

    //htu specific handling
}