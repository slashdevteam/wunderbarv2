#pragma once

#include "resource.h"
#include "PinNames.h"
#include "InterruptIn.h"
#include "Thread.h"

class Button : public Resource
{
public:
    Button(Resources* _resources, const std::string& name, PinName _pin);
    virtual ~Button() {};

    virtual void advertise(IPubSub* _proto) override;
    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    virtual void handleCommand(const char* id, const char* data) override;
    bool parseCommand(const char* data);

private:
    void irqCounter();
    void irqFall();
    void irqRise();
    void clearFlash();
    void waitForRise();
    size_t toJson(char* outputString, size_t maxLen, const uint8_t* data);

private:
    volatile size_t counter;
    mbed::InterruptIn buttonIrq;
    rtos::Thread buttonIrqTiming;
};
