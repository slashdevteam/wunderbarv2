#include "wbgyro.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

using namespace wunderbar::characteristics::sensor;
using AM = AccessMode;

WbGyro::WbGyro(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_GYRO)),
                      randomPassKey(),
                      _resources,
                      _log),
      defaultRateApplied(false)
{
}

void WbGyro::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            publish(mbed::callback(this, &WbGyro::dataToJson), data);
            if(!defaultRateApplied)
            {
                defaultRateApplied = true;
                sendToServer(wunderbar::characteristics::sensor::FREQUENCY,
                             reinterpret_cast<const uint8_t*>(&defaultRate),
                             sizeof(defaultRate));
            }
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
            WunderbarSensor::event(_event, data, len);
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

CharState WbGyro::sensorHasCharacteristic(uint16_t uuid, AccessMode requestedMode)
{
    static const std::list<CharcteristicDescriptor> gyroCharacteristics = {{CONFIG, AM::RW},
                                                                             {FREQUENCY, AM::RW},
                                                                             {THRESHOLD, AM::RW}};
    CharState uuidState = searchCharacteristics(uuid, requestedMode, gyroCharacteristics);

    if(CharState::NOT_FOUND == uuidState)
    {
        uuidState = WunderbarSensor::sensorHasCharacteristic(uuid, requestedMode);
    }

    return uuidState;
}

bool WbGyro::handleWriteUuidRequest(uint16_t uuid, const char* data)
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

bool WbGyro::sendConfig(const char* data)
{
    bool sendOk = false;
    JsonDecode config(data, 8);

    if(config)
    {
        char gyroScaleBuffer[1];
        char accScaleBuffer[1];
        if(config.copyTo("gyroScale", gyroScaleBuffer, 1)
           && config.copyTo("accScale", accScaleBuffer, 1))
        {
            int gyroScale = std::atoi(gyroScaleBuffer);
            int accScale = std::atoi(accScaleBuffer);
            if(isGyroScaleAllowed(gyroScale) && isAccScaleAllowed(accScale))
            {
                config_t scales;
                scales.gyro_full_scale = static_cast<gyroFullScale_t>(gyroScale);
                scales.acc_full_scale = static_cast<accFullScale_t>(accScale);
                sendOk = sendToServer(wunderbar::characteristics::sensor::CONFIG,
                                      reinterpret_cast<uint8_t*>(&scales),
                                      sizeof(scales));

            }
        }
    }
    return sendOk;
}

bool WbGyro::setFrequency(const char* data)
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

bool WbGyro::setThreshold(const char* data)
{
    bool sendOk = false;
    JsonDecode threshold(data, 16);

    if(threshold)
    {
        if(threshold.isField("gyroSbl")
           && threshold.isField("gyroLow")
           && threshold.isField("gyroHigh")
           && threshold.isField("accSbl")
           && threshold.isField("accLow")
           && threshold.isField("accHigh"))
        {
            char thresholdBuffer[12]; // enough for -2147483648 + '\0'
            threshold_t thresholds;
            // need to conserve stack, so char buffer is reused
            threshold.copyTo("gyroSbl", thresholdBuffer, sizeof(thresholdBuffer));
            // sbl is 32 bit unsigned so need to use std::atol
            thresholds.gyro.sbl = static_cast<uint32_t>(std::atol(thresholdBuffer));

            threshold.copyTo("gyroLow", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.gyro.low = static_cast<int32_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("gyroHigh", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.gyro.high = static_cast<int32_t>(std::atoi(thresholdBuffer));


            threshold.copyTo("accSbl", thresholdBuffer, sizeof(thresholdBuffer));
            // sbl is unsigned, but only 16 bits, so std::atoi is enough
            thresholds.acc.sbl = static_cast<uint16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("accLow", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.acc.low = static_cast<int16_t>(std::atoi(thresholdBuffer));

            threshold.copyTo("accHigh", thresholdBuffer, sizeof(thresholdBuffer));
            thresholds.acc.high = static_cast<int16_t>(std::atoi(thresholdBuffer));

            sendOk = sendToServer(wunderbar::characteristics::sensor::THRESHOLD,
                                  reinterpret_cast<uint8_t*>(&thresholds),
                                  sizeof(thresholds));
        }
    }
    return sendOk;
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
                "\"type\":{"
                    "\"type\":\"array\","
                    "\"maxItems\":3,"
                    "\"minItems\":3,"
                    "\"items\":{"
                        "\"type\":\"integer\","
                        "\"min\":-32768,"
                        "\"max\":32767"
                    "}"
                "}"
            "},"
            "{"
                "\"name\":\"accel\","
                "\"type\":{"
                    "\"type\":\"array\","
                    "\"maxItems\":3,"
                    "\"minItems\":3,"
                    "\"items\":{"
                        "\"type\":\"integer\","
                        "\"min\":-32768,"
                        "\"max\":32767"
                    "}"
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
                "\"type\":{"
                    "\"type\":\"array\","
                    "\"maxItems\":2,"
                    "\"minItems\":2,"
                    "\"items\":{"
                        "\"type\":\"integer\","
                        "\"min\":0,"
                        "\"max\":3"
                    "}"
                "}"
            "},"
            "{"
                "\"name\":\"thresholdGyroAcc\","
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
