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
                                 Resources* _resources)
    : BleServer(_gateway,
                std::forward<ServerName>(_name),
                std::forward<PassKey>(_passKey),
                mbed::callback(this, &WunderbarSensor::event)),
      Resource(_resources,
               _name,
               _name),
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
    const char* jsonBattLevel = "{\"batteryLevel\":%d}";
    return snprintf(outputString, maxLen, jsonBattLevel, data);
}

int WunderbarSensor::fwRevToJson(char* outputString, size_t maxLen, const char* data)
{
    const char* jsonSensorFwRev        = "{\"firmware\":\"%s\"}";
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateRawString(buff);

    return snprintf(outputString, maxLen, jsonSensorFwRev, data);
}

int WunderbarSensor::hwRevToJson(char* outputString, size_t maxLen, const char* data)
{
    const char* jsonSensorHwRev        = "{\"hardware\":\"%s\"}";
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateRawString(buff);

    return snprintf(outputString, maxLen, jsonSensorHwRev, data);
}

int WunderbarSensor::manufacturerToJson(char* outputString, size_t maxLen, const char* data)
{
    const char* jsonSensorManufacturer = "{\"manufacturer\":\"%s\"}";
    char buff[MAX_SENSOR_PAYLOAD_LEN];
    memcpy(buff, data, MAX_SENSOR_PAYLOAD_LEN);

    terminateRawString(buff);

    return snprintf(outputString, maxLen, jsonSensorManufacturer, data);
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

size_t WunderbarSensor::getSenseSpec(char* dst, size_t maxLen)
{
    const char sensSpecFormat[] = "{"
        "\"name\":\"batteryLevel\","
        "\"type\":\"integer\","
        "\"min\":0,"
        "\"max\":100"
    "},"
    "{"
        "\"name\":\"firmware\","
        "\"type\":\"String\""
    "},"
    "{"
        "\"name\":\"hardware\","
        "\"type\":\"String\""
    "},"
    "{"
        "\"name\":\"manufacturer\","
        "\"type\":\"String\""
    "}";
 
    return snprintf(dst,
                    maxLen,
                    sensSpecFormat);
}

size_t WunderbarSensor::getActuateSpec(char* dst, size_t maxLen)
{
    return 0;
}
