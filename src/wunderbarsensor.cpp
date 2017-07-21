#include "wunderbarsensor.h"
#include "wunderbarble.h"

WunderbarSensor::WunderbarSensor(IBleGateway& _gateway,
                                 ServerName&& _name,
                                 PassKey&& _passKey,
                                 BleServerCallback _callback)
    : BleServer(_gateway,
                std::forward<ServerName>(_name),
                std::forward<PassKey>(_passKey),
                mbed::callback(this, &WunderbarSensor::wunderbarEvent)),
      sensorCallback(_callback)
{}

void WunderbarSensor::handleDiscovery()
{
    discoveryOk = true;
    gateway.serverDiscoveryComlpete(config);
}

void WunderbarSensor::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    if(registrationOk)
    {
        switch(event)
        {
            case BleEvent::DISCOVERY_COMPLETE:
                handleDiscovery();
                break;
            case BleEvent::DISCOVERY_ERROR:
                discoveryOk = false;
                break;
            default:
                if(sensorCallback)
                {
                    sensorCallback(event, data, len);
                }
                break;
        }
    }
}

