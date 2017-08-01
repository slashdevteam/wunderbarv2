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
                    IPubSub* _proto);
    virtual ~WunderbarSensor() {};

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
