#include "wbhtu.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

using namespace wunderbar::characteristics::sensor;
using AM = AccessMode;

WbHtu::WbHtu(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_HTU)),
                      randomPassKey(),
                      _resources,
                      _log),
      defaultRateApplied(false)
{
}

void WbHtu::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            publish(mbed::callback(this, &WbHtu::dataToJson), data);
            if(!defaultRateApplied)
            {
                defaultRateApplied = true;
                sendToServer(wunderbar::characteristics::sensor::FREQUENCY,
                             reinterpret_cast<const uint8_t*>(&defaultRate),
                             sizeof(defaultRate));
            }
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
            WunderbarSensor::event(_event, data, len);
            break;
    }
}

void WbHtu::handleCommand(const char* id, const char* data)
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
            if(sendConfig(message.get("setConfig")))
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

CharState WbHtu::sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode)
{
    static const std::list<CharcteristicDescriptor> htuCharacteristics = {{CONFIG, AM::RW},
                                                                          {FREQUENCY, AM::RW},
                                                                          {THRESHOLD, AM::RW}};
    CharState uuidState = searchCharacteristics(uuid, requestedMode, htuCharacteristics);

    if(CharState::NOT_FOUND == uuidState)
    {
        uuidState = WunderbarSensor::sensorHasCharacteristic(uuid, requestedMode);
    }

    return uuidState;
}

bool WbHtu::handleWriteUuidRequest(uint16_t uuid, const char* data)
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
                case CONFIG:
                    writeOk = sendConfig(writeValue);
                    break;
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

bool WbHtu::sendConfig(const char* data)
{
    bool sendOk = false;
    JsonDecode config(data, 8);

    if(config)
    {
        char configBuffer[1];
        if(config.copyTo("htuTemp", configBuffer, 1))
        {
            int htuConf = std::atoi(configBuffer);
            if(isConfigAllowed(htuConf))
            {
                sendOk = sendToServer(wunderbar::characteristics::sensor::CONFIG,
                                    reinterpret_cast<uint8_t*>(&htuConf),
                                    sizeof(htuConf));

            }
        }
    }
    return sendOk;
}

bool WbHtu::setFrequency(const char* data)
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

bool WbHtu::setThreshold(const char* data)
{
    bool sendOk = false;
    JsonDecode threshold(data, 16);

    if(threshold)
    {
        if(threshold.isField("tempSbl")
           && threshold.isField("tempLow")
           && threshold.isField("tempHigh")
           && threshold.isField("humSbl")
           && threshold.isField("humLow")
           && threshold.isField("humHigh"))
        {
            char thresholdBuffer[12]; // enough for -2147483648 + '\0'
            threshold_t thresholds;
            // need to conserve stack, so char buffer is reused
            threshold.copyTo("tempSbl", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.temp.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("tempLow", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.temp.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("tempHigh", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.temp.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("humSbl", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.hum.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("humLow", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.hum.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("humHigh", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.hum.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

            sendOk = sendToServer(wunderbar::characteristics::sensor::THRESHOLD,
                                  reinterpret_cast<uint8_t*>(&thresholds),
                                  sizeof(thresholds));
        }
    }
    return sendOk;
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
                "\"type\":{"
                    "\"type\":\"array\","
                    "\"maxItems\":2,"
                    "\"minItems\":2,"
                    "\"items\":{"
                        "\"type\":{"
                            "\"type\":\"array\","
                            "\"maxItems\":3,"
                            "\"minItems\":3,"
                            "\"items\":{"
                                "\"type\":\"integer\","
                                "\"min\":-2147483648,"
                                "\"max\":4294967295"
                            "}"
                        "}"
                    "}"
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
