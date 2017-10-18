#pragma once

#include "bleserver.h"
#include "resource.h"
#include "istdinout.h"
#include "Thread.h"
#include <list>

const size_t MAX_COMMAND_ID_LEN = 20;

using CharacteristicsList = std::list<CharcteristicDescriptor>;

enum class CharState : uint8_t
{
    FOUND_ACCESS_OK = 0,
    FOUND_WRONG_ACCESS = 1,
    NOT_FOUND = 2,
    WRONG_ACCESS = 3
};

class WunderbarSensor : public BleServer, public Resource
{
public:
    WunderbarSensor(IBleGateway& _gateway,
                    ServerName&& _name,
                    PassKey&& _passKey,
                    BleServerCallback _callback,
                    Resources* _resources,
                    IStdInOut& _log);
    virtual ~WunderbarSensor() {};

    virtual void advertise(IPubSub* _proto) override;
    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;

protected:
    // resource command handling
    virtual void handleCommand(const char* id, const char* data) override;
    // WB command handling
    virtual CharState sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode);
    virtual bool handleWriteUuidRequest(uint16_t uuid, const char* data);
    virtual CharState findUuid(const char* data, uint16_t& uuid, AccessMode requestedMode);
    CharState searchCharacteristics(uint16_t uuid, AccessMode requestedMode, const CharacteristicsList& sensorsChars);

private:
    void event(BleEvent _event, const uint8_t* data, size_t len);
    size_t batteryToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t fwRevToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t hwRevToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t manufacturerToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t sensorIdToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t beaconFreqToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t stringLength(const uint8_t* data);



protected:
    int retCode;
    char commandId[MAX_COMMAND_ID_LEN];

private:
    BleServerCallback userCallback;
};
