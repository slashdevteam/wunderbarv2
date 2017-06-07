#pragma once

#include "resource.h"
#include "PinNames.h"
#include "DigitalOut.h"
#include "Thread.h"
#include "PlatformMutex.h"
#include <memory>

class Led : public Resource
{

const int32_t NEW_SUB_SIGNAL = 0x8;

public:
    Led(Protocol* _proto, const std::string& _topic, PinName _pin);

    bool subscribe();
    int32_t read();

private:
    void subscribeThread();
    void subscribeCallback(const char* _command);

    void lock();
    void unlock();

private:
    volatile int32_t state;
    mbed::DigitalOut ledPin;
    rtos::Thread subscriber;
    std::shared_ptr<const char> commandPtr;
    PlatformMutex mutex;
};
