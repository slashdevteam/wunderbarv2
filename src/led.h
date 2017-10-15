#pragma once

#include "resource.h"
#include "PinNames.h"
#include "DigitalOut.h"
#include "Ticker.h"
#include "Thread.h"
#include "istdinout.h"

class Led : public Resource
{
public:
    Led(Resources* _resources, const std::string& _topic, PinName _pin, IStdInOut& _log);
    virtual ~Led();

    virtual void advertise(IPubSub* _proto) override;
    virtual void revoke() override;
    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

    int32_t read();

protected:
    virtual void handleCommand(const char* id, const char* data) override;
    bool parseCommand(const char* data);

private:
    void pingPub();
    void readAndPub();
    size_t toJson(char* outputString, size_t maxLen, const uint8_t* data);

private:
    volatile int32_t state;
    mbed::DigitalOut ledPin;
    mbed::Ticker pubTick;
    rtos::Thread publisher;
    const int READ_AND_PUB = 0x1;
};
