#pragma once

#include "iblegateway.h"

class BleServer
{
public:
    BleServer(IBleGateway& _gateway,
              ServerName&& _name,
              PassKey&& _passKey);
    virtual ~BleServer() = default;

    bool sendToServer(uint16_t bleCharUuid, const uint8_t* data, size_t len);
    bool readFromServer(uint16_t bleCharUuid);

protected:
    virtual void event(BleEvent event, const uint8_t* data, size_t len);

protected:
    BleServerConfig config;
    bool registrationOk;
    bool discoveryOk;
    IBleGateway& gateway;
};
