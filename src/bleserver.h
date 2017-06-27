#pragma once

#include "iblegateway.h"

class BleServer
{
public:
    BleServer(IBleGateway& _gateway,
              ServerName&& _name,
              ServerID _id,
              RequiredServices&& _requiredServices,
              Characteristics&& _characteristics,
              ServerUUID&& _uuid,
              PassKey&& _passKey,
              Security&& _security,
              BleServerCallback _callback);
    virtual ~BleServer();

private:
    IBleGateway& gateway;
    BleServerConfig config;

protected:
    bool registrationOk;

};
