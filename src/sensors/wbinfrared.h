#pragma once

#include "wunderbarsensor.h"

class WbInfraRed : public WunderbarSensor
{
public:
    WbInfraRed(IBleGateway& _gateway, IPubSub* _proto);

private:
    void wunderbarEvent(BleEvent event, const uint8_t* data, size_t len);

    using sensor_ir_data_t = uint8_t;
};
