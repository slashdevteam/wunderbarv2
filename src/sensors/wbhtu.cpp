#include "wbhtu.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

WbHtu::WbHtu(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_HTU)),
                      randomPassKey(),
                      mbed::callback(this, &WbHtu::event),
                      _resources,
                      _log)
{
}

void WbHtu::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            publish(mbed::callback(this, &WbHtu::dataToJson), data);
            break;
        case BleEvent::DATA_SENSOR_FREQUENCY:
            publish(mbed::callback(this, &WbHtu::frequencyToJson), data);
            break;
        case BleEvent::DATA_SENSOR_THRESHOLD:
            publish(mbed::callback(this, &WbHtu::thresholdToJson), data);
            break;
        case BleEvent::DATA_SENSOR_CONFIG:
            publish(mbed::callback(this, &WbHtu::configToJson), data);
            break;
        default:
            break;
    }
}

void WbHtu::handleCommand(const char* id, const char* data)
{
    retCode = 400;
    // first do a pass on common commands
    WunderbarSensor::handleCommand(id, data);

    // if common returned 400 check bridge specific
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
                    && message.isField("tempSbl")
                    && message.isField("tempLow")
                    && message.isField("tempHigh")
                    && message.isField("humSbl")
                    && message.isField("humLow")
                    && message.isField("humHigh"))
            {
                    char thresholdBuffer[12]; // enough for -2147483648 + '\0'
                    threshold_t thresholds;
                    // need to conserve stack, so char buffer is reused
                    message.copyTo("tempSbl", thresholdBuffer, sizeof(thresholdBuffer));
                    // sbl is 32 bit unsigned so need to use std::stol
                    thresholds.temp.sbl = static_cast<uint32_t>(std::atoi(thresholdBuffer));

                    message.copyTo("tempLow", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.temp.low = static_cast<int32_t>(std::atoi(thresholdBuffer));

                    message.copyTo("tempHigh", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.temp.high = static_cast<int32_t>(std::atoi(thresholdBuffer));

                    message.copyTo("humSbl", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.hum.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

                    message.copyTo("humLow", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.hum.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

                    message.copyTo("humHigh", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.hum.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

                    if(sendToServer(wunderbar::characteristics::sensor::THRESHOLD,
                                reinterpret_cast<uint8_t*>(&thresholds),
                                sizeof(thresholds)))
                    {
                        retCode = 200;
                    }
            }
            else if(message.isField("getConfig"))
            {
                if(readFromServer(wunderbar::characteristics::sensor::CONFIG))
                {
                    retCode = 200;
                    acknowledge(id, retCode);
                }
            }
            else if(message.isField("setConfig"))
            {
                char configBuffer[1];
                if(message.copyTo("htuTemp", configBuffer, 1))
                {
                    int config = std::atoi(configBuffer);
                    if(isConfigAllowed(config))
                    {
                        if(sendToServer(wunderbar::characteristics::sensor::CONFIG,
                                        reinterpret_cast<uint8_t*>(&config),
                                        sizeof(config)))
                        {
                            retCode = 200;
                        }
                    }
                }
            }
        }
    }
}

size_t WbHtu::configToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"config\":%d",
                         static_cast<int>(data[0]));
}

size_t WbHtu::thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const threshold_t* thresholds = reinterpret_cast<const threshold_t*>(data);
    return std::snprintf(outputString,
                         maxLen,
                         "\"thresholdTempHum\":[[%d,%d,%d], [%d,%d,%d]]",
                         thresholds->temp.sbl,
                         thresholds->temp.low,
                         thresholds->temp.high,
                         thresholds->hum.sbl,
                         thresholds->hum.low,
                         thresholds->hum.high);
}

size_t WbHtu::frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"frequency\":%d",
                         static_cast<int>(data[0]));
}

bool WbHtu::isConfigAllowed(int config)
{
    bool allowed = false;
    sensor_htu_config_t enumConfig = static_cast<sensor_htu_config_t>(config);
    switch(enumConfig)
    {
        case HTU21D_RH_12_TEMP14:  // intentional fall-through
        case HTU21D_RH_8_TEMP12:   // intentional fall-through
        case HTU21D_RH_10_TEMP13:  // intentional fall-through
        case HTU21D_RH_11_TEMP11:  // intentional fall-through
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }
    return allowed;
}

size_t WbHtu::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"temp\","
                "\"type\":\"integer\","
                "\"min\":-50,"
                "\"max\":100"
            "},"
            "{"
                "\"name\":\"hum\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":100"
            "},"
            "{"
                "\"name\":\"frequency\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":4294967295"
            "},"
            "{"
                "\"name\":\"config\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":3"
            "},"
            "{"
                "\"name\":\"thresholdTempHum\","
                "\"type\":\"array\","
                "\"maxItems\":2,"
                "\"minItems\":2,"
                "\"items\":{"
                    "\"type\":\"array\","
                    "\"maxItems\":3,"
                    "\"minItems\":3,"
                    "\"items\":{"
                        "\"type\":\"integer\","
                        "\"min\":-2147483648,"
                        "\"max\":4294967295"
                    "}"
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

size_t WbHtu::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuateSpecFormatHead[] =
    "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"commands\":"
        "["
            "{"
                "\"CommandName\":\"getConfig\""
            "},"
            "{"
                "\"CommandName\":\"setConfig\","
                "\"DataListe\":"
                "[{"
                    "\"ValueName\":\"htuTemp\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":3"
                "}]"
            "},"
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
                    "\"ValueName\":\"tempSbl\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":65535"
                "},"
                "{"
                    "\"ValueName\":\"tempLow\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"tempHigh\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"humSbl\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":65535"
                "},"
                "{"
                    "\"ValueName\":\"humLow\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"humHigh\","
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
