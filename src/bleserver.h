#pragma once

#include "iblegateway.h"

class BleServer
{
public:
    BleServer(IBleGateway& _gateway,
              ServerName&& _name,
              PassKey&& _passKey,
              BleServerCallback _callback);
    virtual ~BleServer();

private:
    void bleServerEvent(BleEvent event, const uint8_t* data, size_t len);
    void storeMac(const uint8_t* data);

protected:
    BleServerConfig config;
    bool registrationOk;
    bool discoveryOk;
    IBleGateway& gateway;

private:
    BleServerCallback externalCallback;
};
