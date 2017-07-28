
#pragma once

#include "wunderbarsensor.h"

class Bridge : public WunderbarSensor
{
public:
    Bridge(IBleGateway& _gateway, IPubSub* _proto);

private:
    virtual void wunderbarEvent(BleEvent event, uint8_t* data, size_t len) override;
};