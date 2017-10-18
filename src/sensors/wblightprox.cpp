#include "wblightprox.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

using namespace wunderbar::characteristics::sensor;
using AM = AccessMode;

WbLightProx::WbLightProx(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_LIGHT)),
                      randomPassKey(),
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
            WunderbarSensor::event(_event, data, len);
            break;
    }
}

void WbLightProx::handleCommand(const char* id, const char* data)
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

CharState WbLightProx::sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode)
{
    static const std::list<CharcteristicDescriptor> lightProxCharacteristics = {{CONFIG, AM::RW},
                                                                                {FREQUENCY, AM::RW},
                                                                                {THRESHOLD, AM::RW}};
     CharState uuidState = searchCharacteristics(uuid, requestedMode, lightProxCharacteristics);

    if(CharState::NOT_FOUND == uuidState)
    {
        uuidState = WunderbarSensor::sensorHasCharacteristic(uuid, requestedMode);
    }

    return uuidState;
}

bool WbLightProx::handleWriteUuidRequest(uint16_t uuid, const char* data)
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

bool WbLightProx::sendConfig(const char* data)
{
    bool sendOk = false;
    JsonDecode config(data, 8);

    if(config)
    {
        char rgbcGainBuffer[2];
        char proxDriveBuffer[3];

        if(config.copyTo("rgbcGain", rgbcGainBuffer, 2)
           && config.copyTo("proxDrive", proxDriveBuffer, 3))
        {
            int rgbcGain = std::atoi(rgbcGainBuffer);
            int proxDrive = std::atoi(proxDriveBuffer);
            if(isRgbcGainAllowed(rgbcGain) && isProxDriveAllowed(proxDrive))
            {
                sensor_lightprox_config_t lpConfig;
                lpConfig.rgbc_gain = static_cast<rgbc_gain_t>(rgbcGain);
                lpConfig.prox_drive = static_cast<prox_drive_t>(proxDrive);
                sendOk = sendToServer(wunderbar::characteristics::sensor::CONFIG,
                                      reinterpret_cast<uint8_t*>(&lpConfig),
                                      sizeof(lpConfig));

            }
        }
    }
    return sendOk;
}

bool WbLightProx::setFrequency(const char* data)
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

bool WbLightProx::setThreshold(const char* data)
{
    bool sendOk = false;
    JsonDecode threshold(data, 16);

    if(threshold)
    {
        if(threshold.isField("whiteSbl")
           && threshold.isField("whiteLow")
           && threshold.isField("whiteHigh")
           && threshold.isField("proxSbl")
           && threshold.isField("proxLow")
           && threshold.isField("proxHigh"))
        {
            char thresholdBuffer[12]; // enough for -2147483648 + '\0'
            threshold_t thresholds;
            // need to conserve stack, so char buffer is reused
            threshold.copyTo("whiteSbl", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.white.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("whiteLow", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.white.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("whiteHigh", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.white.high = static_cast<int16_t>(std::atoi(thresholdBuffer));


            threshold.copyTo("proxSbl", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.prox.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("proxLow", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.prox.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("proxHigh", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.prox.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

            sendOk = sendToServer(wunderbar::characteristics::sensor::THRESHOLD,
                                  reinterpret_cast<uint8_t*>(&thresholds),
                                  sizeof(thresholds));
        }
    }
    return sendOk;
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
