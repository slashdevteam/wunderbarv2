#pragma once

#include "iblegateway.h"

// Dummy passkey for ongoing development
const PassKey defaultPass = {0x34, 0x36, 0x37, 0x33, 0x36, 0x31, 0x00, 0x00};

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

protected:
    BleServerConfig config;
    bool registrationOk;
    bool discoveryOk;
    IBleGateway& gateway;

private:
    BleServerCallback externalCallback;
};
