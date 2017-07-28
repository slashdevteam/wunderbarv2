
#pragma once

#include "wunderbarsensor.h"

class Htu : public WunderbarSensor
{
public:
    Htu(IBleGateway& _gateway, IPubSub* _proto);

private:
    virtual void wunderbarEvent(BleEvent event, uint8_t* data, size_t len) override;
};