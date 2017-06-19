#pragma once

#include "resource.h"
#include "PinNames.h"
#include "DigitalOut.h"
#include "Thread.h"
#include <memory>

class Led : public Resource
{

const int32_t NEW_SUB_SIGNAL = 0x8;
const int32_t ACK_DONE_SIGNAL = 0x10;

public:
    Led(IPubSub* _proto, const std::string& _topic, PinName _pin);

    bool subscribe();
    int32_t read();

private:
    void subscribeThread();
    void subscribeCallback(const uint8_t* data, size_t len);
    void subscribeDone(bool status);
    void ackDone(bool status);

    void lock();
    void unlock();

private:
    const std::string subtopic;
    const std::string acktopic;
    volatile int32_t state;
    mbed::DigitalOut ledPin;
    rtos::Thread subscriber;
    std::unique_ptr<uint8_t[]> command;
    volatile bool subscribed;
};
