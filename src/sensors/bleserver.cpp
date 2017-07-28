#include "bleserver.h"

#include <cstring>

BleServer::BleServer(IBleGateway& _gateway,
                     ServerName&& _name,
                     PassKey&& _passKey,
                     BleServerCallback _callback)
    : config{_name,
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

void BleServer::bleServerEvent(BleEvent event, uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DISCOVERY_COMPLETE:
            discoveryOk = true;
            gateway.serverDiscoveryComlpete(config);
            break;

        case BleEvent::DISCOVERY_ERROR:
            discoveryOk = false;
            break;

        default:
            break;
    }

    if (externalCallback)
    {
        externalCallback(event, data, len);
    }
}
