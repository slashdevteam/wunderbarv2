#include "wbinfrared.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

WbInfraRed::WbInfraRed(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_IR)),
                      randomPassKey(),
                      mbed::callback(this, &WbInfraRed::event),
                      _resources,
                      _log)
{
}

void WbInfraRed::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        default:
            // IR transmitter has no events
            break;
    }
}

void WbInfraRed::handleCommand(const char* id, const char* data)
{
    retCode = 400;
    // first do a pass on common commands
    WunderbarSensor::handleCommand(id, data);

    // if common returned 400 check ir specific
    if(400 == retCode)
    {
        std::strncpy(commandId, id, MAX_COMMAND_ID_LEN);
        JsonDecode message(data, 16);

        if(message)
        {
            char valueBuffer[1];
            if(message.copyTo("TX", valueBuffer, 1))
            {
                int value = std::atoi(valueBuffer);
                if(value >= 0 || value <= 255)
                {
                    dataDown = value;
                    if(sendToServer(wunderbar::characteristics::sensor::DATA_W,
                                     reinterpret_cast<uint8_t*>(&dataDown),
                                     sizeof(dataDown)))
                    {
                        retCode = 200;
                    }
                }
            }
        }
    }
}

size_t WbInfraRed::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "[";

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

size_t WbInfraRed::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuateSpecFormatHead[] =
    "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"commands\":"
        "["
            "{"
                "\"CommandName\":\"TX\","
                "\"DataListe\":"
                "[{"
                    "\"ValueName\":\"TX\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":255"
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
