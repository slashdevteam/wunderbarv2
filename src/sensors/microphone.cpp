#include "microphone.h"

Microphone::Microphone(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(sensorNameMicrophone),
                      PassKey(defaultPass),
                      mbed::callback(this, &Microphone::wunderbarEvent),
                      _proto)
{
};

void Microphone::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    //base handling
    WunderbarSensor::wunderbarEvent(event, data, len);

    //htu specific handling
}