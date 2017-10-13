#pragma once

#include "bleserver.h"
#include "resource.h"
#include "Thread.h"
#include <list>

const size_t MAX_COMMAND_ID_LEN = 20;

class WunderbarSensor : public BleServer, public Resource
{
public:
    WunderbarSensor(IBleGateway& _gateway,
                    ServerName&& _name,
                    PassKey&& _passKey,
                    BleServerCallback _callback,
                    Resources* _resources);
    virtual ~WunderbarSensor() {};

    virtual void advertise(IPubSub* _proto) override;
    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    virtual void handleCommand(const char* id, const char* data) override;
    void event(BleEvent _event, const uint8_t* data, size_t len);

private:
    size_t batteryToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t fwRevToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t hwRevToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t manufacturerToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t sensorIdToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t beaconFreqToJson(char* outputString, size_t maxLen, const uint8_t* dataa);
    void terminateRawString(char* data);

    // commands
    bool findUuid(const char* data, uint16_t& uuid);
    bool handleBatteryLevelRequest();
    bool handleReadUuidRequest(const char* data);

protected:
    int retCode;
    char commandId[MAX_COMMAND_ID_LEN];

private:
    BleServerCallback userCallback;
};
