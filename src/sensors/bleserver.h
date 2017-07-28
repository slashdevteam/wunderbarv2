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
    void bleServerEvent(BleEvent event, uint8_t* data, size_t len);

protected:
    BleServerConfig config;
    bool registrationOk;
    bool discoveryOk;
    IBleGateway& gateway;

private:
    BleServerCallback externalCallback;
};
