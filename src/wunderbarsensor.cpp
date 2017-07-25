#include "wunderbarsensor.h"
#include "wunderbarble.h"
#include <unordered_map>

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

const std::unordered_map<uint8_t, std::list<uint16_t>> wbSenorChars = {
    {sensors::DATA_ID_DEV_HTU,    {ID, BEACON_FREQ, LED_STATE,
                                   FREQUENCY, THRESHOLD, CONFIG, 
                                   DATA_R} },

    {sensors::DATA_ID_DEV_GYRO,   {ID, BEACON_FREQ, LED_STATE,
                                   FREQUENCY, THRESHOLD, CONFIG, 
                                   DATA_R} },

    {sensors::DATA_ID_DEV_LIGHT,  {ID, BEACON_FREQ, LED_STATE,
                                   FREQUENCY, THRESHOLD, CONFIG, 
                                   DATA_R} },

    {sensors::DATA_ID_DEV_SOUND,  {ID, BEACON_FREQ, LED_STATE,
                                   FREQUENCY, THRESHOLD, DATA_R} },

    {sensors::DATA_ID_DEV_BRIDGE, {ID, BEACON_FREQ, LED_STATE,
                                   CONFIG, DATA_R, DATA_W, } },

    {sensors::DATA_ID_DEV_IR    , {ID, BEACON_FREQ, LED_STATE,
                                   DATA_W} }
};

const std::unordered_map<uint16_t, AccessMode> bleCharsAccessModes {
    {characteristics::sensor::ID,          AccessMode::READ},
    {characteristics::sensor::BEACON_FREQ, AccessMode::RW},
    {characteristics::sensor::FREQUENCY,   AccessMode::RW},
    {characteristics::sensor::LED_STATE,   AccessMode::WRITE},
    {characteristics::sensor::THRESHOLD,   AccessMode::RW},
    {characteristics::sensor::CONFIG,      AccessMode::RW},
    {characteristics::sensor::DATA_R,      AccessMode::READ},
    {characteristics::sensor::DATA_W,      AccessMode::WRITE},
};

const ServerName WunderbarSensorNames[] = {
    "WunderbarHTU",
    "WunderbarGYRO",
    "WunderbarLIGHT",
    "WunderbarMIC",
    "WunderbarBRIDG",
    "WunderbarIR"
};

const std::unordered_map<ServerName, uint8_t> ServerNamesToDataId = {
    {WunderbarSensorNames[0], sensors::DATA_ID_DEV_HTU},
    {WunderbarSensorNames[1], sensors::DATA_ID_DEV_GYRO},
    {WunderbarSensorNames[2], sensors::DATA_ID_DEV_LIGHT},
    {WunderbarSensorNames[3], sensors::DATA_ID_DEV_SOUND},
    {WunderbarSensorNames[4], sensors::DATA_ID_DEV_BRIDGE},
    {WunderbarSensorNames[5], sensors::DATA_ID_DEV_IR}
};

WunderbarSensor::WunderbarSensor(IBleGateway& _gateway,
                                 ServerName&& _name,
                                 PassKey&& _passKey,
                                 BleServerCallback _callback,
                                 IPubSub* _proto)
    : BleServer(_gateway,
                std::forward<ServerName>(_name),
                std::forward<PassKey>(_passKey),
                mbed::callback(this, &WunderbarSensor::wunderbarEvent)),
      sensorCallback(_callback),
      mqttClient(_proto, "actuator/" + _name, "sensor/" + _name),
      bleChars(wbSenorChars.at(ServerNamesToDataId.at(_name)))
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
                mqttClient.handleDeviceEvent(event, data, len);
                if(sensorCallback)
                {
                    sensorCallback(event, data, len);
                }
                break;
        }
    }
}

