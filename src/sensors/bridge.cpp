#include "bridge.h"

Bridge::Bridge(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameBridge),
                        PassKey(defaultPass),
                        mbed::callback(this, &Bridge::wunderbarEvent),
                        _proto)
{
};

void Bridge::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    //base handling
    WunderbarSensor::wunderbarEvent(event, data, len);

    //htu specific handling
}