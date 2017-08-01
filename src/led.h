#pragma once

#include "resource.h"
#include "PinNames.h"
#include "DigitalOut.h"
#include <memory>

class Led : public Resource
{
public:
    Led(IPubSub* _proto, const std::string& _topic, PinName _pin);

    int32_t read();

private:
    volatile int32_t state;
    mbed::DigitalOut ledPin;
};
