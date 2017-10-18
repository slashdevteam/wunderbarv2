#include "bleserver.h"

#include <cstring>

BleServer::BleServer(IBleGateway& _gateway,
                     ServerName&& _name,
                     PassKey&& _passKey)
    : config{_name,
             {0},
             _passKey},
      registrationOk(false),
      discoveryOk(false),
      gateway(_gateway)
{
    registrationOk = gateway.registerServer(config, mbed::callback(this, &BleServer::event));
}

bool BleServer::sendToServer(uint16_t bleCharUuid, const uint8_t* data, size_t len)
{
    return gateway.requestWrite(config, bleCharUuid, data, len);
}

bool BleServer::readFromServer(uint16_t bleCharUuid)
{
    return gateway.requestRead(config, bleCharUuid);
}

void BleServer::event(BleEvent event, const uint8_t* data, size_t len)
{
    switch(event)
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
}
