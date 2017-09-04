#pragma once

#include "resource.h"
#include "PinNames.h"
#include "DigitalOut.h"
#include "Ticker.h"

class Led : public Resource
{
public:
    Led(Resources* _resources, const std::string& _topic, PinName _pin);
    virtual ~Led() {};

    int32_t read();
    virtual const char* getSenseSpec() override;
    virtual const char* getActuateSpec() override;
    virtual void advertise(IPubSub* _proto) override;

protected:
    virtual int handleCommand(const char* command) override;
    bool parseCommand(const char* data);

private:
    void readAndPub();

private:
    volatile int32_t state;
    mbed::DigitalOut ledPin;
    mbed::Ticker pubTick;
};
