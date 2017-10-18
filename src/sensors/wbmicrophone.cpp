#include "wbmicrophone.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

using namespace wunderbar::characteristics::sensor;
using AM = AccessMode;

WbMicrophone::WbMicrophone(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_SOUND)),
                      randomPassKey(),
                      _resources,
                      _log)
{
}

void WbMicrophone::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            publish(mbed::callback(this, &WbMicrophone::dataToJson), data);
            break;
        case BleEvent::DATA_SENSOR_FREQUENCY:
            publish(mbed::callback(this, &WbMicrophone::frequencyToJson), data);
            break;
        case BleEvent::DATA_SENSOR_THRESHOLD:
            publish(mbed::callback(this, &WbMicrophone::thresholdToJson), data);
            break;
        default:
            WunderbarSensor::event(_event, data, len);
            break;
    }
}

void WbMicrophone::handleCommand(const char* id, const char* data)
{
    retCode = 400;

    std::strncpy(commandId, id, MAX_COMMAND_ID_LEN);
    JsonDecode message(data, 16);

    if(message)
    {
        if(message.isField("getFrequency"))
        {
            if(readFromServer(wunderbar::characteristics::sensor::FREQUENCY))
            {
                retCode = 200;
                acknowledge(id, retCode);
            }
        }
        else if(message.isField("setFrequency"))
        {
            if(setFrequency(message.get("setFrequency")))
            {
                retCode = 200;
            }
        }
        else if(message.isField("getThreshold"))
        {
            if(readFromServer(wunderbar::characteristics::sensor::THRESHOLD))
            {
                retCode = 200;
                acknowledge(id, retCode);
            }
        }
        else if(message.isField("setThreshold"))
        {
            if(setThreshold(message.get("setThreshold")))
            {
                retCode = 200;
            }
        }
        else
        {
            WunderbarSensor::handleCommand(id, data);
        }
    }
    else
    {
        acknowledge(id, 400);
    }
}

size_t WbMicrophone::thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const threshold_t* thresholds = reinterpret_cast<const threshold_t*>(data);
    return std::snprintf(outputString,
                         maxLen,
                         "\"threshold\":[%d,%d,%d]",
                         thresholds->mic_level.sbl,
                         thresholds->mic_level.low,
                         thresholds->mic_level.high);
}

size_t WbMicrophone::frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"frequency\":%d",
                         static_cast<int>(data[0]));
}

CharState WbMicrophone::sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode)
{
    static const std::list<CharcteristicDescriptor> micCharacteristics = {{FREQUENCY, AM::RW},
                                                                             {THRESHOLD, AM::RW}};
    CharState uuidState = searchCharacteristics(uuid, requestedMode, micCharacteristics);

    if(CharState::NOT_FOUND == uuidState)
    {
        uuidState = WunderbarSensor::sensorHasCharacteristic(uuid, requestedMode);
    }

    return uuidState;
}

bool WbMicrophone::handleWriteUuidRequest(uint16_t uuid, const char* data)
{
    bool writeOk = false;
    JsonDecode writeRequest(data, 8);

    if(writeRequest)
    {
        char writeValue[20];
        size_t lenght = 0;
        if(writeRequest.copyTo("value", writeValue, sizeof(writeValue)))
        {
            switch(uuid)
            {
                case FREQUENCY:
                    writeOk = setFrequency(writeValue);
                    break;
                case THRESHOLD:
                    writeOk = setThreshold(writeValue);
                    break;
                default:
                    writeOk = WunderbarSensor::handleWriteUuidRequest(uuid, data);
                    break;
            }

            if(writeOk)
            {
                writeOk = sendToServer(uuid,
                          reinterpret_cast<uint8_t*>(writeValue),
                          lenght);
            }
        }
    }

    return writeOk;
}

bool WbMicrophone::setFrequency(const char* data)
{
    bool sendOk = false;
    JsonDecode frequency(data, 8);

    if(frequency)
    {
        char frequencyBuffer[12]; // enough for 4294967295 + '\0'
        if(frequency.copyTo("ticks", frequencyBuffer, sizeof(frequencyBuffer)))
        {
            // frequency is 32 bit unsigned so need to use std::atol
            uint32_t frequency = static_cast<uint32_t>(std::atol(frequencyBuffer));
            sendOk = sendToServer(wunderbar::characteristics::sensor::FREQUENCY,
                                  reinterpret_cast<uint8_t*>(&frequency),
                                  sizeof(frequency));
        }
    }
    return sendOk;
}

bool WbMicrophone::setThreshold(const char* data)
{
    bool sendOk = false;
    JsonDecode threshold(data, 16);

    if(threshold)
    {
        if(threshold.isField("sbl")
          && threshold.isField("low")
          && threshold.isField("high"))
        {
            char thresholdBuffer[12]; // enough for -2147483648 + '\0'
            threshold_t thresholds;
            threshold.copyTo("sbl", thresholdBuffer, sizeof(thresholdBuffer));
            // sbl is unsigned, but only 16 bits, so std::atoi is enough
            thresholds.mic_level.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("low", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.mic_level.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("high", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.mic_level.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

            sendOk = sendToServer(wunderbar::characteristics::sensor::THRESHOLD,
                                  reinterpret_cast<uint8_t*>(&thresholds),
                                  sizeof(thresholds));
        }
    }
    return sendOk;
}

size_t WbMicrophone::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"level\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":65535"
            "},"
            "{"
                "\"name\":\"frequency\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":4294967295"
            "},"
            "{"
                "\"name\":\"threshold\","
                "\"type\":\"array\","
                "\"maxItems\":3,"
                "\"minItems\":3,"
                "\"items\":{"
                    "\"type\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "}"
            "},";

    const char senseSpecFormatTail[] =
        "]"
    "}";

    size_t sizeWritten = std::snprintf(dst,
                                       maxLen,
                                       senseSpecFormatHead,
                                       config.name.c_str(),
                                       config.name.c_str());

    sizeWritten += WunderbarSensor::getSenseSpec(dst + sizeWritten, maxLen - sizeWritten);

    sizeWritten += std::snprintf(dst + sizeWritten,
                                 maxLen - sizeWritten,
                                 senseSpecFormatTail);

    return sizeWritten;
}

size_t WbMicrophone::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuateSpecFormatHead[] =
    "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"commands\":"
        "["
            "{"
                "\"CommandName\":\"getFrequency\""
            "},"
            "{"
                "\"CommandName\":\"setFrequency\","
                "\"DataListe\":"
                "[{"
                    "\"ValueName\":\"ticks\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":4294967295"
                "}]"
            "},"
            "{"
                "\"CommandName\":\"getThreshold\""
            "},"
            "{"
                "\"CommandName\":\"setThreshold\","
                "\"DataListe\":"
                "[{"
                    "\"ValueName\":\"sbl\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":65535"
                "},"
                "{"
                    "\"ValueName\":\"low\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"high\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "}]"
            "},";

    const char actuateSpecFormatTail[] =
        "]"
    "}";

    size_t sizeWritten = std::snprintf(dst,
                                       maxLen,
                                       actuateSpecFormatHead,
                                       config.name.c_str(),
                                       config.name.c_str());

    sizeWritten += WunderbarSensor::getActuateSpec(dst + sizeWritten, maxLen - sizeWritten);

    sizeWritten += std::snprintf(dst + sizeWritten,
                                 maxLen - sizeWritten,
                                 actuateSpecFormatTail);

    return sizeWritten;
}
