#include "bleserver.h"

BleServer::BleServer(IBleGateway& _gateway,
                     ServerName&& _name,
                     ServerID _id,
                     RequiredServices&& _requiredServices,
                     Characteristics&& _characteristics,
                     ServerUUID&& _uuid,
                     PassKey&& _passKey,
                     Security&& _security,
                     BleServerCallback _callback)
    : gateway(_gateway),
      config{_name,
             _id,
             _requiredServices,
             _characteristics,
             _uuid,
             _passKey,
             _security}

{
    registrationOk = gateway.registerServer(config, _callback);
}

BleServer::~BleServer()
{

}
