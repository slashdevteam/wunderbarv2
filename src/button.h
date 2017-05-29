#pragma once

#include "resource.h"
#include "PinNames.h"
#include "InterruptIn.h"
#include "Thread.h"

class Button : public Resource
{

const int32_t BUTTON_IRQ_SIGNAL = 0x4;

public:
    Button(Protocol* _proto, const std::string& _topic, PinName _pin);

private:
    void irqCounter();
    void publishThread();

private:
    volatile size_t counter;
    mbed::InterruptIn buttonIrq;
    rtos::Thread publisher;
};
