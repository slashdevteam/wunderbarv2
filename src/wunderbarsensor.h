#pragma once


#include "bleserver.h"
#include "Thread.h"

#include <unordered_map>

using CharcteristicData = std::unordered_map<uint16_t, uint8_t*>;

class WunderbarSensor : public BleServer
{
public:
    WunderbarSensor(IBleGateway& _gateway,
                    ServerName&& _name,
                    PassKey&& _passKey,
                    BleServerCallback _callback);
    virtual ~WunderbarSensor() {};

private:
    bool subscribe();
    void handleDiscovery();
    void wunderbarEvent(BleEvent event, const uint8_t* data, size_t len);

private:
    BleServerCallback sensorCallback;
};
