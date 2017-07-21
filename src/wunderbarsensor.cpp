#include "wunderbarsensor.h"
#include "wunderbarble.h"
#include <unordered_map>
#include <list>

// List of all available characteristics for sensor types
using CharDesc = CharcteristicDescriptor;
using namespace wunderbar;
using namespace wunderbar::characteristics::sensor;

using AM = AccessMode;

const std::unordered_map<uint8_t, std::list<CharDesc> > WbChars = {
    {sensors::DATA_ID_DEV_HTU,    {CharDesc(ID, AM::READ), CharDesc(BEACON_FREQ, AM::RW), CharDesc(LED_STATE, AM::WRITE), 
                                   CharDesc(FREQUENCY, AM::RW), CharDesc(THRESHOLD, AM::RW), CharDesc(CONFIG, AM::RW), 
                                   CharDesc(DATA_R, AM::READ)} },

    {sensors::DATA_ID_DEV_GYRO,   {CharDesc(ID, AM::READ), CharDesc(BEACON_FREQ, AM::RW), CharDesc(LED_STATE, AM::WRITE), 
                                   CharDesc(FREQUENCY, AM::RW), CharDesc(THRESHOLD, AM::RW), CharDesc(CONFIG, AM::RW), 
                                   CharDesc(DATA_R, AM::READ)} },

    {sensors::DATA_ID_DEV_LIGHT,  {CharDesc(ID, AM::READ), CharDesc(BEACON_FREQ, AM::RW), CharDesc(LED_STATE, AM::WRITE), 
                                   CharDesc(FREQUENCY, AM::RW), CharDesc(THRESHOLD, AM::RW), CharDesc(CONFIG, AM::RW), 
                                   CharDesc(DATA_R, AM::READ)} },

    {sensors::DATA_ID_DEV_SOUND,  {CharDesc(ID, AM::READ), CharDesc(BEACON_FREQ, AM::RW), CharDesc(LED_STATE, AM::WRITE), 
                                   CharDesc(FREQUENCY, AM::RW), CharDesc(THRESHOLD, AM::RW), CharDesc(DATA_R, AM::READ)} },

    {sensors::DATA_ID_DEV_BRIDGE, {CharDesc(ID, AM::READ), CharDesc(BEACON_FREQ, AM::RW), CharDesc(LED_STATE, AM::WRITE), 
                                   CharDesc(CONFIG, AM::RW), CharDesc(DATA_R, AM::READ), CharDesc(DATA_W, AM::WRITE)} },

    {sensors::DATA_ID_DEV_IR    , {CharDesc(ID, AM::READ), CharDesc(BEACON_FREQ, AM::RW), CharDesc(LED_STATE, AM::WRITE),
                                   CharDesc(DATA_W, AM::WRITE)} }
};

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

