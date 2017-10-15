#include "wunderbarsensor.h"
#include "wunderbarble.h"
#include <unordered_map>
#include "wunderbarsensordatatypes.h"
#include <cstdint>
#include <cstdio>
#include "mbed.h"
#include "jsondecode.h"

// List of all available characteristics for sensor types
using namespace wunderbar::characteristics::sensor;
using namespace wunderbar::characteristics::ble;
using AM = AccessMode;

const uint16_t INVALID_UUID = 0xFFFF;

bool sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode)
{
    bool uuidFound = false;
    static const std::list<CharcteristicDescriptor> commonSensorChars = {{ID, AM::READ},
                                                                         {BEACON_FREQ, AM::RW},
                                                                         {LED_STATE, AM::WRITE},
                                                                         {BATTERY_LEVEL, AM::READ},
                                                                         {MANUFACTURER_NAME, AM::READ},
                                                                         {HARDWARE_REVISION, AM::READ},
                                                                         {FIRMWARE_REVISION, AM::READ}};
    if(AM::NONE != requestedMode)
    {
        for(auto& characteristic : commonSensorChars)
        {
            if((characteristic.uuid == uuid)
                && ((requestedMode == characteristic.mode) || (characteristic.mode == AM::RW)))
            {
                uuidFound = true;
                break;
            }
        }
    }

    return uuidFound;
}

WunderbarSensor::WunderbarSensor(IBleGateway& _gateway,
                                 ServerName&& _name,
                                 PassKey&& _passKey,
                                 BleServerCallback _callback,
                                 Resources* _resources,
                                 IStdInOut& _log)
    : BleServer(_gateway,
                std::forward<ServerName>(_name),
                std::forward<PassKey>(_passKey),
                mbed::callback(this, &WunderbarSensor::event)),
      Resource(_resources,
               _name,
               _name,
               _log),
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
            if(readFromServer(wunderbar::characteristics::ble::BATTERY_LEVEL))
            {
                retCode = 200;
                sendAckImmediately = true;
            }
        }
        else if(message.isField("getManufacturer"))
        {
            if(readFromServer(wunderbar::characteristics::ble::MANUFACTURER_NAME))
            {
                retCode = 200;
                sendAckImmediately = true;
            }
        }
        else if(message.isField("getFirmware"))
        {
            if(readFromServer(wunderbar::characteristics::ble::FIRMWARE_REVISION))
            {
                retCode = 200;
                sendAckImmediately = true;
            }
        }
        else if(message.isField("getHardware"))
        {
            if(readFromServer(wunderbar::characteristics::ble::HARDWARE_REVISION))
            {
                retCode = 200;
                sendAckImmediately = true;
            }
        }
        else if(message.isField("getBeaconFreq"))
        {
            if(readFromServer(wunderbar::characteristics::sensor::BEACON_FREQ))
            {
                retCode = 200;
                sendAckImmediately = true;
            }
        }
        else if(message.isField("getSensorId"))
        {
            if(readFromServer(wunderbar::characteristics::sensor::ID))
            {
                retCode = 200;
                sendAckImmediately = true;
            }
        }
        else if(message.isField("blinkLed"))
        {
            uint8_t value = 1;
            if(sendToServer(wunderbar::characteristics::sensor::LED_STATE, &value, 1))
            {
                retCode = 200;
                sendAckImmediately = false;
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
        else if(message.isField("writeUUID"))
        {
            char writeRequestBuffer[50];
            if(message.copyTo("writeUUID", writeRequestBuffer, sizeof(writeRequestBuffer)))
            {
                if(handleWriteUuidRequest(writeRequestBuffer))
                {
                    retCode = 200;
                    sendAckImmediately = false;
                }
            }
        }
    }
    if(sendAckImmediately)
    {
        acknowledge(commandId, retCode);
    }
}

bool WunderbarSensor::findUuid(const char* data, uint16_t& uuid, AccessMode requestedMode)
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
                if(sensorHasCharacteristic(uuidNumber, requestedMode))
                {
                    uuid = uuidNumber;
                }
            }
            else if(0 == std::strncmp(uuidNameType, "integer", 7))
            {
                uint16_t uuidNumber = std::atoi(uuidName);
                if(sensorHasCharacteristic(uuidNumber, requestedMode))
                {
                    uuid = uuidNumber;
                }
            }
            else if(0 == std::strncmp(uuidNameType, "name", 4))
            {
                if(0 == std::strncmp(uuidName, "batteryLevel", 12)
                   && requestedMode == AM::READ)
                {
                    uuid = wunderbar::characteristics::ble::BATTERY_LEVEL;
                }
                else if(0 == std::strncmp(uuidName, "firmware", 8)
                        && requestedMode == AM::READ)
                {
                    uuid = wunderbar::characteristics::ble::FIRMWARE_REVISION;
                }
                else if(0 == std::strncmp(uuidName, "hardware", 8)
                        && requestedMode == AM::READ)
                {
                    uuid = wunderbar::characteristics::ble::HARDWARE_REVISION;
                }
                else if(0 == std::strncmp(uuidName, "manufacturer", 12)
                        && requestedMode == AM::READ)
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

bool WunderbarSensor::handleReadUuidRequest(const char* data)
{
    bool readOk = false;
    uint16_t uuid = INVALID_UUID;

    if(findUuid(data, uuid, AM::READ))
    {
        readOk = readFromServer(uuid);
    }

    return readOk;
}

bool WunderbarSensor::handleWriteUuidRequest(const char* data)
{
    bool writeOk = false;
    uint16_t uuid = INVALID_UUID;

    if(findUuid(data, uuid, AM::WRITE))
    {
        JsonDecode writeRequest(data, 8);
        if(writeRequest)
        {
            char writeValue[20];
            if(writeRequest.copyTo("value", writeValue, sizeof(writeValue)))
            {
                writeOk = sendToServer(uuid,
                                       reinterpret_cast<uint8_t*>(writeValue),
                                       std::strlen(writeValue));
            }
        }

    }

    return writeOk;
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
        "\"CommandName\":\"getFirmware\""
    "},"
    "{"
        "\"CommandName\":\"getHardware\""
    "},"
    "{"
        "\"CommandName\":\"getManufacturer\""
    "},"
    "{"
        "\"CommandName\":\"getSensorId\""
    "},"
    "{"
        "\"CommandName\":\"getBeaconFreq\""
    "},"
    "{"
        "\"CommandName\":\"blinkLed\""
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
            "\"ValueDescription\":\"hex,integer,name\""
        "}]"
    "},"
    "{"
        "\"CommandName\":\"writeUUID\","
        "\"DataListe\":"
        "[{"
            "\"ValueName\":\"uuid\","
            "\"ValueType\":\"String\""
        "},"
        "{"
            "\"ValueName\":\"uuidType\","
            "\"ValueType\":\"String\","
            "\"ValueDescription\":\"hex,integer,name\""
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
