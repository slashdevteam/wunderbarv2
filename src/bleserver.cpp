#include "bleserver.h"

#include <cstring>

BleServer::BleServer(IBleGateway& _gateway,
                     ServerName&& _name,
                     ServerHandle _handle,
                     PassKey&& _passKey,
                     BleServerCallback _callback)
    : config{_handle,
             _name,
             {0},
             _passKey},
      registrationOk(false),
      discoveryOk(false),
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
