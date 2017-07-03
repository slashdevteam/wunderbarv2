#pragma once

#include "iblegateway.h"

class BleServer
{
public:
    BleServer(IBleGateway& _gateway,
              ServerName&& _name,
              ServerHandle _handle,
              RequiredServices&& _requiredServices,
              ServerUUID&& _uuid,
              PassKey&& _passKey,
              Security&& _security,
              BleServerCallback _callback);
    virtual ~BleServer();

private:
    void bleServerEvent(BleEvent event, const uint8_t* data, size_t len);
    void storeMac(const uint8_t* data);

protected:
    BleServerConfig config;
    bool registrationOk;
    size_t discoveryCharacteristicIdx;
    size_t discoveryServiceIdx;
    bool discoveryOk;
    IBleGateway& gateway;

private:
    BleServerCallback externalCallback;
};
