#pragma once

#include "wunderbarsensor.h"

class WbInfraRed : public WunderbarSensor
{

using sensor_ir_data_t = uint8_t;

public:
    WbInfraRed(IBleGateway& _gateway, Resources* _resources);
    virtual ~WbInfraRed() = default;

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;
    virtual void advertise(IPubSub* _proto) override;

protected:
    virtual int handleCommand(const char* command) override;
    bool parseCommand(const char* data);

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);

private:
    sensor_ir_data_t dataDown;
};
