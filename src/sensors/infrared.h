#pragma once

#include "wunderbarsensor.h"

class InfraRed : public WunderbarSensor
{
public:
    InfraRed(IBleGateway& _gateway, IPubSub* _proto);

private:
    void wunderbarEvent(BleEvent event, uint8_t* data, size_t len);

    using sensor_ir_data_t = uint8_t;
};
