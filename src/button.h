#pragma once

#include "resource.h"
#include "PinNames.h"
#include "InterruptIn.h"

class Button : public Resource
{
public:
    Button(IPubSub* _proto, const std::string& _topic, PinName _pin);
    virtual ~Button() {};

private:
    void irqCounter();

private:
    volatile size_t counter;
    mbed::InterruptIn buttonIrq;
};
