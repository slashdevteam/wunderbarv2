#include "wbgyro.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

WbGyro::WbGyro(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_GYRO)),
                      randomPassKey(),
                      mbed::callback(this, &WbGyro::event),
                      _resources,
                      _log)
{
}

void WbGyro::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            publish(mbed::callback(this, &WbGyro::dataToJson), data);
            break;
        case BleEvent::DATA_SENSOR_FREQUENCY:
            publish(mbed::callback(this, &WbGyro::frequencyToJson), data);
            break;
        case BleEvent::DATA_SENSOR_THRESHOLD:
            publish(mbed::callback(this, &WbGyro::thresholdToJson), data);
            break;
        case BleEvent::DATA_SENSOR_CONFIG:
            publish(mbed::callback(this, &WbGyro::configToJson), data);
            break;
        default:
            break;
    }
}

void WbGyro::handleCommand(const char* id, const char* data)
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
                && message.isField("gyroSbl")
                && message.isField("gyroLow")
                && message.isField("gyroHigh")
                && message.isField("accSbl")
                && message.isField("accLow")
                && message.isField("accHigh"))
        {
                char thresholdBuffer[12]; // enough for -2147483648 + '\0'
                threshold_t thresholds;
                // need to conserve stack, so char buffer is reused
                message.copyTo("gyroSbl", thresholdBuffer, sizeof(thresholdBuffer));
                // sbl is 32 bit unsigned so need to use std::stol
                thresholds.gyro.sbl = static_cast<uint32_t>(std::atol(thresholdBuffer));

                message.copyTo("gyroLow", thresholdBuffer, sizeof(thresholdBuffer));
                thresholds.gyro.low = static_cast<int32_t>(std::atoi(thresholdBuffer));

                message.copyTo("gyroHigh", thresholdBuffer, sizeof(thresholdBuffer));
                thresholds.gyro.high = static_cast<int32_t>(std::atoi(thresholdBuffer));


                message.copyTo("accSbl", thresholdBuffer, sizeof(thresholdBuffer));
                // sbl is unsigned, but only 16 bits, so std::atoi is enough
                thresholds.acc.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

                message.copyTo("accLow", thresholdBuffer, sizeof(thresholdBuffer));
                thresholds.acc.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

                message.copyTo("accHigh", thresholdBuffer, sizeof(thresholdBuffer));
                thresholds.acc.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

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
            char gyroScaleBuffer[1];
            char accScaleBuffer[1];

            if(message.copyTo("gyroScale", gyroScaleBuffer, 1)
               && message.copyTo("accScale", accScaleBuffer, 1))
            {
                int gyroScale = std::atoi(gyroScaleBuffer);
                int accScale = std::atoi(accScaleBuffer);
                if(isGyroScaleAllowed(gyroScale) && isAccScaleAllowed(accScale))
                {
                    config_t scales;
                    scales.gyro_full_scale = static_cast<gyroFullScale_t>(gyroScale);
                    scales.acc_full_scale = static_cast<accFullScale_t>(accScale);
                    if(sendToServer(wunderbar::characteristics::sensor::CONFIG,
                                    reinterpret_cast<uint8_t*>(&scales),
                                    sizeof(scales)))
                    {
                        retCode = 200;
                    }
                }
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

size_t WbGyro::configToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"scale\":{\"gyro\": %d, \"acc\":%d}",
                         static_cast<int>(data[0]),
                         static_cast<int>(data[1]));
}

size_t WbGyro::thresholdToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const threshold_t* thresholds = reinterpret_cast<const threshold_t*>(data);
    return std::snprintf(outputString,
                         maxLen,
                         "\"thresholdGyroAcc\":[[%ld,%ld,%ld], [%d,%d,%d]]",
                         thresholds->gyro.sbl,
                         thresholds->gyro.low,
                         thresholds->gyro.high,
                         thresholds->acc.sbl,
                         thresholds->acc.low,
                         thresholds->acc.high);
}

size_t WbGyro::frequencyToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"frequency\":%d",
                         static_cast<int>(data[0]));
}

bool WbGyro::isGyroScaleAllowed(int scale)
{
    bool allowed = false;
    gyroFullScale_t enumScale = static_cast<gyroFullScale_t>(scale);
    switch(enumScale)
    {
        case GYRO_FULL_SCALE_250DPS:   // intentional fall-through
        case GYRO_FULL_SCALE_500DPS:   // intentional fall-through
        case GYRO_FULL_SCALE_1000DPS:  // intentional fall-through
        case GYRO_FULL_SCALE_2000DPS:  // intentional fall-through
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }
    return allowed;
}

bool WbGyro::isAccScaleAllowed(int scale)
{
    bool allowed = false;
    accFullScale_t enumScale = static_cast<accFullScale_t>(scale);
    switch(enumScale)
    {
        case ACC_FULL_SCALE_2G:   // intentional fall-through
        case ACC_FULL_SCALE_4G:   // intentional fall-through
        case ACC_FULL_SCALE_8G:   // intentional fall-through
        case ACC_FULL_SCALE_16G:  // intentional fall-through
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }
    return allowed;
}

size_t WbGyro::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"gyro\","
                "\"type\":\"array\","
                "\"maxItems\":3,"
                "\"minItems\":3,"
                "\"items\":{"
                    "\"type\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "}"
            "},"
            "{"
                "\"name\":\"accel\","
                "\"type\":\"array\","
                "\"maxItems\":3,"
                "\"minItems\":3,"
                "\"items\":{"
                    "\"type\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "}"
            "},"
            "{"
                "\"name\":\"frequency\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":4294967295"
            "},"
            "{"
                "\"name\":\"scale\","
                "\"type\":\"array\","
                "\"maxItems\":2,"
                "\"minItems\":2,"
                "\"items\":{"
                    "\"type\":\"integer\","
                    "\"min\":0,"
                    "\"max\":3"
                "}"
            "},"
            "{"
                "\"name\":\"thresholdGyroAcc\","
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

size_t WbGyro::getActuateSpec(char* dst, size_t maxLen)
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
                    "\"ValueName\":\"gyroScale\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":3"
                "},"
                "{"
                    "\"ValueName\":\"accScale\","
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
                    "\"ValueName\":\"gyroSbl\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":4294967295"
                "},"
                "{"
                    "\"ValueName\":\"gyroLow\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-2147483648,"
                    "\"max\":2147483647"
                "},"
                "{"
                    "\"ValueName\":\"gyroHigh\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-2147483648,"
                    "\"max\":2147483647"
                "},"
                "{"
                    "\"ValueName\":\"accSbl\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":65535"
                "},"
                "{"
                    "\"ValueName\":\"accLow\","
                    "\"ValueType\":\"integer\","
                    "\"min\":-32768,"
                    "\"max\":32767"
                "},"
                "{"
                    "\"ValueName\":\"accHigh\","
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
