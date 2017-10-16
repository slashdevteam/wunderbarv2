#pragma once

#include "resource.h"
#include "PinNames.h"
#include "InterruptIn.h"
#include "Thread.h"
#include "EventQueue.h"
#include "flash.h"

class Button : public Resource
{
public:
    Button(Flash& _flash, Resources* _resources, const std::string& name, PinName _pin, IStdInOut& _log);
    virtual ~Button() {};

    virtual void advertise(IPubSub* _proto) override;
    virtual void revoke() override;
    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    virtual void handleCommand(const char* id, const char* data) override;
    bool parseCommand(const char* data);

private:
    void irqFall();
    void irqRise();
    void clearFlash();
    void waitForRise();
    size_t toJson(char* outputString, size_t maxLen, const uint8_t* data);

private:
    Flash& flash;
    bool publishing;
    volatile size_t counter;
    mbed::InterruptIn buttonIrq;
    rtos::Thread buttonIrqTiming;
    const int32_t BUTTON_PRESSED = 0x1;
    const int32_t BUTTON_RELEASED = 0x2;
    const int BUTTON_RESET_TIME_MS = 5000;
};
