#pragma once

#include "bleserver.h"
#include "Thread.h"
#include "mqttclient.h"
#include <list>

class WunderbarSensor : public BleServer
{
public:
    WunderbarSensor(IBleGateway& _gateway,
                    ServerName&& _name,
                    PassKey&& _passKey,
                    BleServerCallback _callback,
                    IPubSub* _proto);
    virtual ~WunderbarSensor() {};

private:
    bool subscribe();
    void handleDiscovery();
    void wunderbarEvent(BleEvent event, const uint8_t* data, size_t len);

private:
    BleServerCallback sensorCallback;
    MqttClient        mqttClient;

    const std::list<uint16_t>& bleChars;
};
