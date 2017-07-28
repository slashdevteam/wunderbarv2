#include "lightprox.h"

LightProx::LightProx(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(sensorNameLightProx),
                      PassKey(defaultPass),
                      mbed::callback(this, &LightProx::wunderbarEvent),
                      _proto)
{
};

void LightProx::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    //base handling
    WunderbarSensor::wunderbarEvent(event, data, len);

    //htu specific handling
}