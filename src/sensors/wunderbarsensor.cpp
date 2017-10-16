#include "wunderbarsensor.h"
#include "wunderbarble.h"
#include <unordered_map>
#include "wunderbarsensordatatypes.h"
#include <cstdint>
#include <cstdio>
#include "mbed.h"
#include "jsondecode.h"

// List of all available characteristics for sensor types
using CharDesc = CharcteristicDescriptor;
using namespace wunderbar;
using namespace wunderbar::characteristics::sensor;

using AM = AccessMode;

const uint16_t INVALID_UUID = 0xFFFF;

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
                                       FREQUENCY, CONFIG, DATA_R, DATA_W } },

        {sensors::DATA_ID_DEV_IR    , {ID, BEACON_FREQ, LED_STATE,
                                       FREQUENCY, DATA_W} }
    };
    return sensorChars.at(static_cast<uint8_t>(sensor));
}

bool sensorHasCharacteristic(const ServerName& name, uint16_t uuid)
{
    bool uuidFound = false;
    DataId sensor = ServerNamesToDataId(name);
    const std::list<uint16_t>& availableChars = wbSensorChars(sensor);

    for(auto charUuid : availableChars)
    {
        if(charUuid == uuid)
        {
            uuidFound = true;
            break;
        }
    }

    return uuidFound;
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
      retCode(400),
      userCallback(_callback)
{}

void WunderbarSensor::advertise(IPubSub* _proto)
{
    Resource::advertise(_proto);
    Resource::subscribe();
}

void WunderbarSensor::event(BleEvent _event, const uint8_t* data, size_t len)
{
    if(registrationOk)
    {
        // handle common events
        switch(_event)
        {
            case BleEvent::DATA_BATTERY_LEVEL:
                publish(mbed::callback(this, &WunderbarSensor::batteryToJson), data);
                break;
            case BleEvent::DATA_MANUFACTURER_NAME:
                publish(mbed::callback(this, &WunderbarSensor::manufacturerToJson), data);
                break;
            case BleEvent::DATA_HARDWARE_REVISION:
                publish(mbed::callback(this, &WunderbarSensor::hwRevToJson), data);
                break;
            case BleEvent::DATA_FIRMWARE_REVISION:
                publish(mbed::callback(this, &WunderbarSensor::fwRevToJson), data);
                break;
            case BleEvent::DATA_SENSOR_ID:
                publish(mbed::callback(this, &WunderbarSensor::sensorIdToJson), data);
                break;
            case BleEvent::DATA_SENSOR_BEACON_FREQUENCY:
                publish(mbed::callback(this, &WunderbarSensor::beaconFreqToJson), data);
                break;
            case BleEvent::WRITE_OK:
                acknowledge(commandId, retCode);
                break;
            default:
                break;
        }

        // handle sensor-specific events
        if(userCallback)
        {
            userCallback(_event, data, len);
        }
    }
}

void WunderbarSensor::handleCommand(const char* id, const char* data)
{
    retCode = 400; // Bad Request
    bool sendAckImmediately = false;
    JsonDecode message(data, 32);
    std::strncpy(commandId, id, MAX_COMMAND_ID_LEN);

    if(message)
    {
        if(message.isField("getBatteryLevel"))
        {
            if(handleBatteryLevelRequest())
            {
                retCode = 200;
                sendAckImmediately = true;
            }
        }
        else if(message.isField("readUUID"))
        {
            char readRequestBuffer[50];
            if(message.copyTo("readUUID", readRequestBuffer, sizeof(readRequestBuffer)))
            {
                if(handleReadUuidRequest(readRequestBuffer))
                {
                    retCode = 200;
                    sendAckImmediately = true;
                }
            }
        }
    }
    if(sendAckImmediately)
    {
        acknowledge(commandId, retCode);
    }
}

bool WunderbarSensor::findUuid(const char* data, uint16_t& uuid)
{
    uuid = INVALID_UUID;
    JsonDecode uuidData(data, 8);
    if(uuidData)
    {
        char uuidName[30];
        char uuidNameType[10];
        if(uuidData.copyTo("uuidType", uuidNameType, sizeof(uuidNameType))
           && uuidData.copyTo("uuid", uuidName, sizeof(uuidName)))
        {
            if(0 == std::strncmp(uuidNameType, "hex", 3))
            {
                uint16_t uuidNumber = INVALID_UUID;
                std::sscanf(uuidName, "0x%hx", &uuidNumber);
                if(sensorHasCharacteristic(config.name, uuidNumber))
                {
                    uuid = uuidNumber;
                }
            }
            else if(0 == std::strncmp(uuidNameType, "integer", 7))
            {
                uint16_t uuidNumber = std::atoi(uuidName);
                if(sensorHasCharacteristic(config.name, uuidNumber))
                {
                    uuid = uuidNumber;
                }
            }
            else if(0 == std::strncmp(uuidNameType, "name", 4))
            {
                if(0 == std::strncmp(uuidName, "batteryLevel", 12))
                {
                    uuid = wunderbar::characteristics::ble::BATTERY_LEVEL;
                }
                else if(0 == std::strncmp(uuidName, "firmware", 8))
                {
                    uuid = wunderbar::characteristics::ble::FIRMWARE_REVISION;
                }
                else if(0 == std::strncmp(uuidName, "hardware", 8))
                {
                    uuid = wunderbar::characteristics::ble::HARDWARE_REVISION;
                }
                else if(0 == std::strncmp(uuidName, "manufacturer", 12))
                {
                    uuid = wunderbar::characteristics::ble::MANUFACTURER_NAME;
                }
                else if(0 == std::strncmp(uuidName, "beaconFreq", 10))
                {
                    uuid = wunderbar::characteristics::sensor::BEACON_FREQ;
                }
                else if(0 == std::strncmp(uuidName, "led", 3))
                {
                    uuid = wunderbar::characteristics::sensor::LED_STATE;
                }
            }
        }
    }

    return (uuid != INVALID_UUID);
}

bool WunderbarSensor::handleBatteryLevelRequest()
{
    return readFromServer(wunderbar::characteristics::ble::BATTERY_LEVEL);
}

bool WunderbarSensor::handleReadUuidRequest(const char* data)
{
    bool readOk = false;
    uint16_t uuid = INVALID_UUID;

    if(findUuid(data, uuid))
    {
        readOk = readFromServer(uuid);
    }

    return readOk;
}

size_t WunderbarSensor::batteryToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"batteryLevel\":%d",
                         static_cast<int>(data[0]));
}

size_t WunderbarSensor::fwRevToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"firmware\":\"%.*s\"",
                         stringLength(data),
                         reinterpret_cast<const char*>(data));
}

size_t WunderbarSensor::hwRevToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"hardware\":\"%.*s\"",
                         stringLength(data),
                         reinterpret_cast<const char*>(data));
}

size_t WunderbarSensor::manufacturerToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"manufacturer\":\"%.*s\"",
                         stringLength(data),
                         reinterpret_cast<const char*>(data));
}

size_t WunderbarSensor::sensorIdToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const size_t SENSOR_ID_LEN = 16;
    const char* jsonSensorIdStart = "\"sensorId\":\"";
    size_t written = std::snprintf(outputString, maxLen, jsonSensorIdStart);

    for(size_t idPart = 0; idPart < SENSOR_ID_LEN; ++idPart)
    {
        written += std::snprintf(outputString + written,
                                 maxLen - written,
                                 " 0x%x",
                                 *(data + idPart));
    }

    written += std::snprintf(outputString + written, maxLen - written, "\"");
    return written;
}

size_t WunderbarSensor::beaconFreqToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"beaconFreq\":%d",
                         static_cast<int>(data[0]));
}

size_t WunderbarSensor::stringLength(const uint8_t* data)
{
    size_t length = 0;
    for(size_t nChar = 0; nChar < MAX_SENSOR_PAYLOAD_LEN; ++nChar)
    {
        if(0xFF == data[nChar])
        {
            length = nChar;
            break;
        }
    }
    return length;
}

size_t WunderbarSensor::getSenseSpec(char* dst, size_t maxLen)
{
    const char sensSpecFormat[] =
    "{"
        "\"name\":\"batteryLevel\","
        "\"type\":\"integer\","
        "\"min\":0,"
        "\"max\":100"
    "},"
    "{"
        "\"name\":\"sensorId\","
        "\"type\":\"String\""
    "},"
    "{"
        "\"name\":\"beaconFreq\","
        "\"type\":\"integer\""
    "},"
    "{"
        "\"name\":\"led\","
        "\"type\":\"boolean\""
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
    const char sensSpecFormat[] =
    "{"
        "\"CommandName\":\"getBatteryLevel\""
    "},"
    "{"
        "\"CommandName\":\"readUUID\","
        "\"DataListe\":"
        "[{"
            "\"ValueName\":\"uuid\","
            "\"ValueType\":\"String\""
        "},"
        "{"
            "\"ValueName\":\"uuidType\","
            "\"ValueType\":\"String\","
            "\"ValueDescription\":\"hex, integer, name\""
        "}]"
    "},"
    "{"
        "\"CommandName\":\"setLed\","
        "\"DataListe\":"
        "[{"
            "\"ValueName\":\"state\","
            "\"ValueType\":\"integer\","
            "\"min\":0,"
            "\"max\":1"
        "}]"
    "},"
    "{"
        "\"CommandName\":\"setUUID\","
        "\"DataListe\":"
        "[{"
            "\"ValueName\":\"uuid\","
            "\"ValueType\":\"String\""
        "},"
        "{"
            "\"ValueName\":\"uuidType\","
            "\"ValueType\":\"String\","
            "\"ValueDescription\":\"hex, integer, name\""
        "},"
        "{"
            "\"ValueName\":\"value\","
            "\"ValueType\":\"String\""
        "}]"
    "}";

    return snprintf(dst,
                    maxLen,
                    sensSpecFormat);
}
