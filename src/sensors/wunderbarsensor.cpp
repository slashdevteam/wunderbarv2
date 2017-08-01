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

const std::list<uint16_t>& wbSensorChars(DataId sensor)
{
    static const std::unordered_map<uint8_t, std::list<uint16_t>> sensorChars =
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
    return sensorChars.at(static_cast<uint8_t>(sensor));
}

WunderbarSensor::WunderbarSensor(IBleGateway& _gateway,
                                 ServerName&& _name,
                                 PassKey&& _passKey,
                                 BleServerCallback _callback,
                                 IPubSub* _proto)
    : BleServer(_gateway,
                std::forward<ServerName>(_name),
                std::forward<PassKey>(_passKey),
                mbed::callback(this, &WunderbarSensor::event)),
      Resource(_proto,
              "actuator/" + _name,
              "sensor/" + _name),
      userCallback(_callback)
{}

void WunderbarSensor::event(BleEvent _event, const uint8_t* data, size_t len)
{
    if (registrationOk)
    {
        // handle common events
        switch(_event)
        {
            case BleEvent::DATA_BATTERY_LEVEL:
                batteryToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, static_cast<int>(data[0]));
                publish();
                break;
            case BleEvent::DATA_MANUFACTURER_NAME:
                manufacturerToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, reinterpret_cast<const char*>(data));
                publish();
                break;
            case BleEvent::DATA_HARDWARE_REVISION:
                hwRevToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, reinterpret_cast<const char*>(data));
                publish();
                break;
            case BleEvent::DATA_FIRMWARE_REVISION:
                fwRevToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, reinterpret_cast<const char*>(data));
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
            userCallback(_event, data, len);
        }
    }
}

int WunderbarSensor::batteryToJson(char* outputString, size_t maxLen, int data)
{
    const char* jsonBattLevel = "{\"ts\":%ld,\"val\":%d}";
    return snprintf(outputString, maxLen, jsonBattLevel, time(NULL), data);
}

int WunderbarSensor::fwRevToJson(char* outputString, size_t maxLen, const char* data)
{
    const char* jsonSensorFwRev        = "{\"ts\":%ld,\"firmware\":\"%s\"}";
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateRawString(buff);

    return snprintf(outputString, maxLen, jsonSensorFwRev, time(NULL), data);
}

int WunderbarSensor::hwRevToJson(char* outputString, size_t maxLen, const char* data)
{
    const char* jsonSensorHwRev        = "{\"ts\":%ld,\"hardware\":\"%s\"}";
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateRawString(buff);

    return snprintf(outputString, maxLen, jsonSensorHwRev, time(NULL), data);
}

int WunderbarSensor::manufacturerToJson(char* outputString, size_t maxLen, const char* data)
{
    const char* jsonSensorManufacturer = "{\"ts\":%ld,\"manufacturer\":\"%s\"}";
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateRawString(buff);

    return snprintf(outputString, maxLen, jsonSensorManufacturer, time(NULL), data);
}

void WunderbarSensor::terminateRawString(char* data)
{
    // find first not valid char and change it to string termination
    for (uint32_t nChar = 0; nChar < MAX_SENSOR_PAYLOAD_LEN; ++nChar)
    {
        if (0xFF == data[nChar])
        {
            data[nChar] = '\0';
        }
    }
}
