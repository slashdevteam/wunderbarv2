#include "wunderbarsensor.h"
#include "wunderbarble.h"
#include <unordered_map>
#include "wunderbarsensordatatypes.h"
#include <cstdint>
#include <cstdio>
#include "mbed.h"

// List of all available characteristics for sensor types
using CharDesc = CharcteristicDescriptor;
using namespace wunderbar;
using namespace wunderbar::characteristics::sensor;

using AM = AccessMode;

const std::unordered_map<uint8_t, std::list<CharDesc> > WbChars
{
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

const std::unordered_map<uint8_t, std::list<uint16_t>> wbSenorChars
{
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

const std::unordered_map<uint16_t, AccessMode> bleCharsAccessMode
{
    {characteristics::sensor::ID,             AccessMode::READ},
    {characteristics::sensor::BEACON_FREQ,    AccessMode::RW},
    {characteristics::sensor::FREQUENCY,      AccessMode::RW},
    {characteristics::sensor::LED_STATE,      AccessMode::WRITE},
    {characteristics::sensor::THRESHOLD,      AccessMode::RW},
    {characteristics::sensor::CONFIG,         AccessMode::RW},
    {characteristics::sensor::DATA_R,         AccessMode::READ},
    {characteristics::sensor::DATA_W,         AccessMode::WRITE},

    {characteristics::ble::BATTERY_LEVEL,     AccessMode::READ},
    {characteristics::ble::MANUFACTURER_NAME, AccessMode::READ},
    {characteristics::ble::HARDWARE_REVISION, AccessMode::READ},
    {characteristics::ble::FIRMWARE_REVISION, AccessMode::READ}
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
      Resource(_proto, 
              "actuator/" + _name, 
              "sensor/" + _name),
      bleChars(wbSenorChars.at(ServerNamesToDataId.at(_name))),
      userCallback(_callback)
{}

void WunderbarSensor::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    if (registrationOk)
    {
        // handle common events
        switch(event)
        {
            case BleEvent::DATA_BATTERY_LEVEL:
                createJsonBattLevel(publishContent, MQTT_MSG_PAYLOAD_SIZE, static_cast<int>(data[0]));
                publish();
            break;

            case BleEvent::DATA_MANUFACTURER_NAME:
                createJsonSensorManufacturer(publishContent, MQTT_MSG_PAYLOAD_SIZE, reinterpret_cast<const char*>(data));
                publish();
            break;

            case BleEvent::DATA_HARDWARE_REVISION:
                createJsonSensorHwRev(publishContent, MQTT_MSG_PAYLOAD_SIZE, reinterpret_cast<const char*>(data));
                publish();
            break;

            case BleEvent::DATA_FIRMWARE_REVISION:
                createJsonSensorFwRev(publishContent, MQTT_MSG_PAYLOAD_SIZE, reinterpret_cast<const char*>(data));
                publish();
            break;

            case BleEvent::DATA_SENSOR_ID:
                // not used yet
            break;

            case BleEvent::DATA_SENSOR_BEACON_FREQUENCY:
                // not used yet
            break;

            case BleEvent::WRITE_OK:
                writeDone();
            break;

            default:
            break;
        }

        // handle sensor-specific events
        if (userCallback)
        {
            userCallback(event, data, len);
        }
    }
}

int WunderbarSensor::createJsonBattLevel(char* outputString, size_t maxLen, int data)
{
    return snprintf(outputString, maxLen, jsonMqttBattLevelFormat, time(NULL), data);
}

void WunderbarSensor::terminateFwHwRawString(char* data)
{
    // find first not valid char and change it to string termination
    for (uint32_t nChar = 0; nChar < MAX_SENSOR_PAYLOAD_LEN; ++nChar)
    {
        if (0xFF == data[nChar])
        {
            data[nChar] = 0;
        }
    }
}

int WunderbarSensor::createJsonSensorFwRev(char* outputString, size_t maxLen, const char* data)
{
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateFwHwRawString(buff);

    return snprintf(outputString, maxLen, jsonMqttSensorFwRevFormat, time(NULL), data);
}

int WunderbarSensor::createJsonSensorHwRev(char* outputString, size_t maxLen, const char* data)
{
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateFwHwRawString(buff);

    return snprintf(outputString, maxLen, jsonMqttSensorHwRevFormat, time(NULL), data);
}

int WunderbarSensor::createJsonSensorManufacturer(char* outputString, size_t maxLen, const char* data)
{
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateFwHwRawString(buff);

    return snprintf(outputString, maxLen, jsonMqttSensorManufacturerFormat, time(NULL), data);
}

