#pragma once

#include "bleserver.h"
#include "Thread.h"
#include "../mqttclient.h"
#include <list>
#include "wunderbarsensordatatypes.h"

class WunderbarSensor : public BleServer
{
public:
    WunderbarSensor(IBleGateway& _gateway,
                    ServerName&& _name,
                    PassKey&& _passKey,
                    BleServerCallback _callback,
                    IPubSub* _proto);
    virtual ~WunderbarSensor() {};

protected:
    virtual void wunderbarEvent(BleEvent event, uint8_t* data, size_t len) = 0;

    MqttClient                 mqttClient;
    const std::list<uint16_t>& bleChars;
private:
    int createJsonBattLevel(char* outputString, size_t maxLen, int data);

    void terminateFwHwRawString(char* data);

    int createJsonSensorFwRev(char* outputString, size_t maxLen, char* data);

    int createJsonSensorHwRev(char* outputString, size_t maxLen, char* data);

    int createJsonSensorManufacturer(char* outputString, size_t maxLen, char* data);

    BleServerCallback userCallback;

    // strings common to all sensors
    const char* jsonMqttBattLevelFormat          = "{\"ts\":%ld,\"val\":%d}";
    const char* jsonMqttSensorFwRevFormat        = "{\"ts\":%ld,\"firmware\":\"%s\"}";
    const char* jsonMqttSensorHwRevFormat        = "{\"ts\":%ld,\"hardware\":\"%s\"}";
    const char* jsonMqttSensorManufacturerFormat = "{\"ts\":%ld,\"manufacturer\":\"%s\"}";

    const uint32_t maxRevStringLen = 12;
};
