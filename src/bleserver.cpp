#include "bleserver.h"

#include <cstring>

BleServer::BleServer(IBleGateway& _gateway,
                     ServerName&& _name,
                     ServerHandle _handle,
                     RequiredServices&& _requiredServices,
                     ServerUUID&& _uuid,
                     PassKey&& _passKey,
                     Security&& _security,
                     BleServerCallback _callback)
    : config{_handle,
             _name,
             {0},
             _requiredServices,
             _uuid,
             _passKey,
             _security},
      registrationOk(false),
      discoveryCharacteristicIdx(0),
      discoveryServiceIdx(0),
      discoveryOk(true),
      gateway(_gateway),
      externalCallback(_callback)
{
    registrationOk = gateway.registerServer(config, mbed::callback(this, &BleServer::bleServerEvent));
}

BleServer::~BleServer()
{
}

void BleServer::storeMac(const uint8_t* data)
{
    ServerIdentificator serverId(data);

    if(serverId.name == config.name)
    {
        std::memcpy(&config.mac, serverId.mac, sizeof(ServerGapAddress));
    }
}

void BleServer::bleServerEvent(BleEvent event, const uint8_t* data, size_t len)
{
    if(event == BleEvent::DISCOVERY_COMPLETE)
    {
        storeMac(data);
    }
    if(externalCallback)
    {
        externalCallback(event, data, len);
    }
}
