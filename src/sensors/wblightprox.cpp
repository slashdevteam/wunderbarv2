#include "wblightprox.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

WbLightProx::WbLightProx(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_LIGHT)),
                      randomPassKey(),
                      mbed::callback(this, &WbLightProx::event),
                      _resources,
                      _log)
{
}

void WbLightProx::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            publish(mbed::callback(this, &WbLightProx::dataToJson), data);
            break;
        case BleEvent::DATA_SENSOR_FREQUENCY:
            publish(mbed::callback(this, &WbLightProx::frequencyToJson), data);
            break;
        case BleEvent::DATA_SENSOR_THRESHOLD:
            publish(mbed::callback(this, &WbLightProx::thresholdToJson), data);
            break;
        case BleEvent::DATA_SENSOR_CONFIG:
            publish(mbed::callback(this, &WbLightProx::configToJson), data);
            break;
        default:
            break;
    }
}

void WbLightProx::handleCommand(const char* id, const char* data)
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
                    && message.isField("whiteSbl")
                    && message.isField("whiteLow")
                    && message.isField("whiteHigh")
                    && message.isField("proxSbl")
                    && message.isField("proxLow")
                    && message.isField("proxHigh"))
            {
                    char thresholdBuffer[12]; // enough for -2147483648 + '\0'
                    threshold_t thresholds;
                    // need to conserve stack, so char buffer is reused
                    message.copyTo("whiteSbl", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.white.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

                    message.copyTo("whiteLow", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.white.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

                    message.copyTo("whiteHigh", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.white.high = static_cast<int16_t>(std::atoi(thresholdBuffer));


                    message.copyTo("proxSbl", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.prox.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

                    message.copyTo("proxLow", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.prox.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

                    message.copyTo("proxHigh", thresholdBuffer, sizeof(thresholdBuffer));
                    thresholds.prox.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

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
                char rgbcGainBuffer[2];
                char proxDriveBuffer[3];

                if(message.copyTo("rgbcGain", rgbcGainBuffer, 2)
                   && message.copyTo("proxDrive", proxDriveBuffer, 3))
                {
                    int rgbcGain = std::atoi(rgbcGainBuffer);
                    int proxDrive = std::atoi(proxDriveBuffer);
                    if(isRgbcGainAllowed(rgbcGain) && isProxDriveAllowed(proxDrive))
                    {
                        sensor_lightprox_config_t config;
                        config.rgbc_gain = static_cast<rgbc_gain_t>(rgbcGain);
                        config.prox_drive = static_cast<prox_drive_t>(proxDrive);
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

size_t WbLightProx::configToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"rgbcGain\":%d,\"proxDrive\":%d",
                         static_cast<int>(data[0]),
                         static_cast<int>(data[1]));
}

size_t WbLightProx::thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const threshold_t* thresholds = reinterpret_cast<const threshold_t*>(data);
    return std::snprintf(outputString,
                         maxLen,
                         "\"thresholdWhiteProx\":[[%d,%d,%d], [%d,%d,%d]]",
                         thresholds->white.sbl,
                         thresholds->white.low,
                         thresholds->white.high,
                         thresholds->prox.sbl,
                         thresholds->prox.low,
                         thresholds->prox.high);
}

size_t WbLightProx::frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"frequency\":%d",
                         static_cast<int>(data[0]));
}

bool WbLightProx::isRgbcGainAllowed(int gain)
{
    bool allowed = false;
    rgbc_gain_t enumGain = static_cast<rgbc_gain_t>(gain);
    switch(enumGain)
    {
        case RGBC_GAIN_1:   // intentional fall-through
        case RGBC_GAIN_4:   // intentional fall-through
        case RGBC_GAIN_16:  // intentional fall-through
        case RGBC_GAIN_60:  // intentional fall-through
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }
    return allowed;
}

bool WbLightProx::isProxDriveAllowed(int drive)
{
    bool allowed = false;
    prox_drive_t enumDrive = static_cast<prox_drive_t>(drive);
    switch(enumDrive)
    {
        case PROX_DRIVE_12_5_MA:   // intentional fall-through
        case PROX_DRIVE_25_MA:     // intentional fall-through
        case PROX_DRIVE_50_MA:     // intentional fall-through
        case PROX_DRIVE_100_MA:    // intentional fall-through
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }
    return allowed;
}

size_t WbLightProx::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"red\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":65535"
            "},"
            "{"
                "\"name\":\"green\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":65535"
            "},"
            "{"
                "\"name\":\"blue\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":65535"
            "},"
            "{"
                "\"name\":\"white\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":65535"
            "},"
            "{"
                "\"name\":\"proximity\","
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
                "\"name\":\"rgbcGain\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":3"
            "},"
            "{"
                "\"name\":\"proxDrive\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":128"
            "},"
            "{"
                "\"name\":\"thresholdWhiteProx\","
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

size_t WbLightProx::getActuateSpec(char* dst, size_t maxLen)
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
                    "\"ValueName\":\"rgbcGain\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":3"
                "},"
                "{"
                    "\"ValueName\":\"proxDrive\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":128"
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
                    "\"ValueName\":\"whiteSbl\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":65535"
                "},"
                "{"
                    "\"ValueName\":\"whiteLow\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"whiteHigh\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"proxSbl\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":65535"
                "},"
                "{"
                    "\"ValueName\":\"proxLow\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"proxHigh\","
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
