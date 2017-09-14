#pragma once

#include "resource.h"
#include "PinNames.h"
#include "InterruptIn.h"

class Button : public Resource
{
public:
    Button(Resources* _resources, const std::string& name, PinName _pin);
    virtual ~Button() {};

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    virtual int handleCommand(const char* command) override;
    bool parseCommand(const char* data);

private:
    void irqCounter();

private:
    volatile size_t counter;
    mbed::InterruptIn buttonIrq;
};
