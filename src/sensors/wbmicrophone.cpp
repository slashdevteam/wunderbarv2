#include "wbmicrophone.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

WbMicrophone::WbMicrophone(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_SOUND)),
                      randomPassKey(),
                      mbed::callback(this, &WbMicrophone::event),
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
            break;
    }
}

void WbMicrophone::handleCommand(const char* id, const char* data)
{
    retCode = 400;
    // first do a pass on common commands
    WunderbarSensor::handleCommand(id, data);

    // if common returned 400 check mic specific
    if(400 == retCode)
    {
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
                char frequencyBuffer[12]; // enough for 4294967295 + '\0'
                if(message.copyTo("ticks", frequencyBuffer, sizeof(frequencyBuffer)))
                {
                    // frequency is 32 bit unsigned so need to use std::stol
                    uint32_t frequency = static_cast<uint32_t>(std::atol(frequencyBuffer));
                    if(sendToServer(wunderbar::characteristics::sensor::FREQUENCY,
                                    reinterpret_cast<uint8_t*>(&frequency),
                                    sizeof(frequency)))
                    {
                        retCode = 200;
                    }
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
            else if(message.isField("setThreshold")
                    && message.isField("sbl")
                    && message.isField("low")
                    && message.isField("high"))
            {
                char thresholdBuffer[12]; // enough for -2147483648 + '\0'
                threshold_t thresholds;
                message.copyTo("sbl", thresholdBuffer, sizeof(thresholdBuffer));
                // sbl is unsigned, but only 16 bits, so std::atoi is enough
                thresholds.mic_level.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

                message.copyTo("low", thresholdBuffer, sizeof(thresholdBuffer));
                thresholds.mic_level.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

                message.copyTo("high", thresholdBuffer, sizeof(thresholdBuffer));
                thresholds.mic_level.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

                if(sendToServer(wunderbar::characteristics::sensor::THRESHOLD,
                            reinterpret_cast<uint8_t*>(&thresholds),
                            sizeof(thresholds)))
                {
                    retCode = 200;
                }
            }
        }
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

    size_t sizeWritten = snprintf(dst,
                                  maxLen,
                                  senseSpecFormatHead,
                                  config.name.c_str(),
                                  config.name.c_str());

    sizeWritten += WunderbarSensor::getSenseSpec(dst + sizeWritten, maxLen - sizeWritten);

    sizeWritten += snprintf(dst + sizeWritten,
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

    size_t sizeWritten = snprintf(dst,
                                  maxLen,
                                  actuateSpecFormatHead,
                                  config.name.c_str(),
                                  config.name.c_str());

    sizeWritten += WunderbarSensor::getActuateSpec(dst + sizeWritten, maxLen - sizeWritten);

    sizeWritten += snprintf(dst + sizeWritten,
                            maxLen - sizeWritten,
                            actuateSpecFormatTail);

    return sizeWritten;
}
