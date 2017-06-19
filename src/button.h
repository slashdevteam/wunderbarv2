#pragma once

#include "resource.h"
#include "PinNames.h"
#include "InterruptIn.h"
#include "Thread.h"

class Button : public Resource
{

const int32_t BUTTON_IRQ_SIGNAL = 0x4;
const int32_t PUBLISH_DONE_SIGNAL = 0x8;

public:
    Button(IPubSub* _proto, const std::string& _topic, PinName _pin);
    virtual ~Button() {};

    void publishDone(bool status);

private:
    void irqCounter();
    void publishThread();

private:
    const std::string pubtopic;
    volatile size_t counter;
    mbed::InterruptIn buttonIrq;
    rtos::Thread publisher;
};
