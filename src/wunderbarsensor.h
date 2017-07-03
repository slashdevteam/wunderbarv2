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
                    BleService&& _wunderbarRunService,
                    ServerUUID&& _uuid,
                    PassKey&& _passKey,
                    BleServerCallback _callback);
    virtual ~WunderbarSensor() {};

private:
    bool verifyData(uint16_t characteristic, const uint8_t* data, size_t len);
    size_t getWriteDataForCharacteristic(uint16_t characteristic, uint8_t*& data);
    void handleDiscoveryCharacteristic(BleEvent event, const uint8_t* data, size_t len);
    bool subscribe();
    void handleDiscovery();
    void wunderbarEvent(BleEvent event, const uint8_t* data, size_t len);

private:
    BleServerCallback sensorCallback;
    uint16_t uuidUnderVerification;
};
