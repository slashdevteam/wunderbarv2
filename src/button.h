#pragma once

#include "resource.h"
#include "PinNames.h"
#include "InterruptIn.h"

class Button : public Resource
{
public:
    Button(Resources* _resources, const std::string& name, PinName _pin);
    virtual ~Button() {};

    virtual const char* getSenseSpec() override;
    virtual const char* getActuateSpec() override;

protected:
    virtual int handleCommand(const char* command) override;
    bool parseCommand(const char* data);

private:
    void irqCounter();

private:
    volatile size_t counter;
    mbed::InterruptIn buttonIrq;
};
