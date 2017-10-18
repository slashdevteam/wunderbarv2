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

WunderbarSensor::WunderbarSensor(IBleGateway& _gateway,
                                 ServerName&& _name,
                                 PassKey&& _passKey,
                                 Resources* _resources,
                                 IStdInOut& _log)
    : BleServer(_gateway,
                std::forward<ServerName>(_name),
                std::forward<PassKey>(_passKey)),
      Resource(_resources,
               _name,
               _name,
               _log),
      retCode(400)
{
}

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
                BleServer::event(_event, data, len);
                break;
        }
    }
}

void WunderbarSensor::handleCommand(const char* id, const char* data)
{
    retCode = 404; // Not Found
    bool sendAckImmediately = true;
    JsonDecode message(data, 32);
    std::strncpy(commandId, id, MAX_COMMAND_ID_LEN);

    if(message)
    {
        if(message.isField("getBatteryLevel"))
        {
            retCode = 400;
            if(readFromServer(wunderbar::characteristics::ble::BATTERY_LEVEL))
            {
                retCode = 200;
            }
        }
        else if(message.isField("getManufacturer"))
        {
            retCode = 400;
            if(readFromServer(wunderbar::characteristics::ble::MANUFACTURER_NAME))
            {
                retCode = 200;
            }
        }
        else if(message.isField("getFirmware"))
        {
            retCode = 400;
            if(readFromServer(wunderbar::characteristics::ble::FIRMWARE_REVISION))
            {
                retCode = 200;
            }
        }
        else if(message.isField("getHardware"))
        {
            retCode = 400;
            if(readFromServer(wunderbar::characteristics::ble::HARDWARE_REVISION))
            {
                retCode = 200;
            }
        }
        else if(message.isField("getBeaconFreq"))
        {
            retCode = 400;
            if(readFromServer(wunderbar::characteristics::sensor::BEACON_FREQ))
            {
                retCode = 200;
            }
        }
        else if(message.isField("getSensorId"))
        {
            retCode = 400;
            if(readFromServer(wunderbar::characteristics::sensor::ID))
            {
                retCode = 200;
            }
        }
        else if(message.isField("blinkLed"))
        {
            retCode = 400;
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
                uint16_t uuid = INVALID_UUID;
                CharState state = findUuid(data, uuid, AM::READ);
                switch(state)
                {
                    case CharState::FOUND_ACCESS_OK:
                        if(readFromServer(uuid))
                        {
                            retCode = 200;
                        }
                        break;
                    case CharState::NOT_FOUND:
                        retCode = 400;
                        break;
                    case CharState::FOUND_WRONG_ACCESS:
                        retCode = 405;
                        break;
                    case CharState::WRONG_ACCESS:
                        retCode = 406;
                        break;
                    default:
                        retCode = 400;
                        break;
                }
            }
        }
        else if(message.isField("writeUUID"))
        {
            char writeRequestBuffer[50];
            if(message.copyTo("writeUUID", writeRequestBuffer, sizeof(writeRequestBuffer)))
            {
                uint16_t uuid = INVALID_UUID;
                CharState state = findUuid(data, uuid, AM::WRITE);
                switch(state)
                {
                    case CharState::FOUND_ACCESS_OK:
                        if(handleWriteUuidRequest(uuid, writeRequestBuffer))
                        {
                            retCode = 200;
                            sendAckImmediately = false;
                        }
                        break;
                    case CharState::NOT_FOUND:
                        retCode = 400;
                        break;
                    case CharState::FOUND_WRONG_ACCESS:
                        retCode = 405;
                        break;
                    case CharState::WRONG_ACCESS:
                        retCode = 406;
                        break;
                    default:
                        retCode = 400;
                        break;
                }
            }
        }
    }
    if(sendAckImmediately)
    {
        acknowledge(commandId, retCode);
    }
}

CharState WunderbarSensor::searchCharacteristics(uint16_t uuid, AccessMode requestedMode, const CharacteristicsList& sensorsChars)
{
    CharState uuidState = CharState::WRONG_ACCESS;
    if(AM::NONE != requestedMode)
    {
        uuidState = CharState::NOT_FOUND;
        for(auto& characteristic : sensorsChars)
        {
            if(characteristic.uuid == uuid)
            {
                uuidState = CharState::FOUND_WRONG_ACCESS;
                if((requestedMode == characteristic.mode) || (characteristic.mode == AM::RW))
                {
                    uuidState = CharState::FOUND_ACCESS_OK;
                    break;
                }
            }
        }
    }
    return uuidState;
}

CharState WunderbarSensor::sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode)
{
    static const std::list<CharcteristicDescriptor> commonSensorChars = {{ID, AM::READ},
                                                                         {BEACON_FREQ, AM::RW},
                                                                         {LED_STATE, AM::WRITE},
                                                                         {BATTERY_LEVEL, AM::READ},
                                                                         {MANUFACTURER_NAME, AM::READ},
                                                                         {HARDWARE_REVISION, AM::READ},
                                                                         {FIRMWARE_REVISION, AM::READ}};
    return searchCharacteristics(uuid, requestedMode, commonSensorChars);
}

CharState WunderbarSensor::findUuid(const char* data, uint16_t& uuid, AccessMode requestedMode)
{
    CharState state = CharState::NOT_FOUND;
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
                state = sensorHasCharacteristic(uuidNumber, requestedMode);
                if(CharState::FOUND_ACCESS_OK == state)
                {
                    uuid = uuidNumber;
                }
            }
            else if(0 == std::strncmp(uuidNameType, "integer", 7))
            {
                uint16_t uuidNumber = std::atoi(uuidName);
                state = sensorHasCharacteristic(uuidNumber, requestedMode);
                if(CharState::FOUND_ACCESS_OK == state)
                {
                    uuid = uuidNumber;
                }
            }
            else if(0 == std::strncmp(uuidNameType, "name", 4))
            {
                state = CharState::WRONG_ACCESS;
                if(requestedMode != AM::NONE)
                {
                    state = CharState::FOUND_WRONG_ACCESS;
                    if(0 == std::strncmp(uuidName, "batteryLevel", 12)
                       && requestedMode == AM::READ)
                    {
                        state = CharState::FOUND_ACCESS_OK;
                        uuid = wunderbar::characteristics::ble::BATTERY_LEVEL;
                    }
                    else if(0 == std::strncmp(uuidName, "firmware", 8)
                            && requestedMode == AM::READ)
                    {
                        state = CharState::FOUND_ACCESS_OK;
                        uuid = wunderbar::characteristics::ble::FIRMWARE_REVISION;
                    }
                    else if(0 == std::strncmp(uuidName, "hardware", 8)
                            && requestedMode == AM::READ)
                    {
                        state = CharState::FOUND_ACCESS_OK;
                        uuid = wunderbar::characteristics::ble::HARDWARE_REVISION;
                    }
                    else if(0 == std::strncmp(uuidName, "sensorId", 8)
                            && requestedMode == AM::READ)
                    {
                        state = CharState::FOUND_ACCESS_OK;
                        uuid = wunderbar::characteristics::sensor::ID;
                    }
                    else if(0 == std::strncmp(uuidName, "manufacturer", 12)
                            && requestedMode == AM::READ)
                    {
                        state = CharState::FOUND_ACCESS_OK;
                        uuid = wunderbar::characteristics::ble::MANUFACTURER_NAME;
                    }
                    else if(0 == std::strncmp(uuidName, "beaconFreq", 10))
                    {
                        state = CharState::FOUND_ACCESS_OK;
                        uuid = wunderbar::characteristics::sensor::BEACON_FREQ;
                    }
                    else if(0 == std::strncmp(uuidName, "led", 3))
                    {
                        state = CharState::FOUND_ACCESS_OK;
                        uuid = wunderbar::characteristics::sensor::LED_STATE;
                    }
                }
            }
        }
    }

    return state;
}

bool WunderbarSensor::handleWriteUuidRequest(uint16_t uuid, const char* data)
{
    bool writeOk = false;
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

    return std::snprintf(dst,
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

    return std::snprintf(dst,
                         maxLen,
                         sensSpecFormat);
}
