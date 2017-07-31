#include "wbinfrared.h"

WbInfraRed::WbInfraRed(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(sensorNameInfraRed),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbInfraRed::wunderbarEvent),
                      _proto)
{
};

void WbInfraRed::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    switch (event)
    {
        default:
        break;
    }
}