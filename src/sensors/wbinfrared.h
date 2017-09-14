#pragma once

#include "wunderbarsensor.h"

class WbInfraRed : public WunderbarSensor
{

using sensor_ir_data_t = uint8_t;

public:
    WbInfraRed(IBleGateway& _gateway, Resources* _resources);

    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
};
