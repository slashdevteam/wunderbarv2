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
    void wunderbarEvent(BleEvent event, const uint8_t* data, size_t len);

    BleServerCallback userCallback;
    MqttClient        mqttClient;

    const std::list<uint16_t>& bleChars;
};
