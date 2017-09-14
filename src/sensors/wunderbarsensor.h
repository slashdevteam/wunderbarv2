#pragma once

#include "bleserver.h"
#include "resource.h"
#include "Thread.h"
#include <list>

class WunderbarSensor : public BleServer, public Resource
{
public:
    WunderbarSensor(IBleGateway& _gateway,
                    ServerName&& _name,
                    PassKey&& _passKey,
                    BleServerCallback _callback,
                    Resources* _resources);
    virtual ~WunderbarSensor() {};

    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

    const char* getSenseSpec();

protected:
    void event(BleEvent _event, const uint8_t* data, size_t len);

private:
    int batteryToJson(char* outputString, size_t maxLen, int data);
    int fwRevToJson(char* outputString, size_t maxLen, const char* data);
    int hwRevToJson(char* outputString, size_t maxLen, const char* data);
    int manufacturerToJson(char* outputString, size_t maxLen, const char* data);
    void terminateRawString(char* data);

private:
    BleServerCallback userCallback;
};
