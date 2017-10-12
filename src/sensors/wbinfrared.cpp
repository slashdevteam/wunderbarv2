#include "wbinfrared.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"
#include "randompasskey.h"
#include "jsondecode.h"

WbInfraRed::WbInfraRed(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_IR)),
                      randomPassKey(),
                      mbed::callback(this, &WbInfraRed::event),
                      _resources)
{
}

void WbInfraRed::advertise(IPubSub* _proto)
{
    Resource::advertise(_proto);
    Resource::startSubscriber();
}

void WbInfraRed::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        default:
          break;
    }
}

int WbInfraRed::handleCommand(const char* data)
{
    int retCode = 400; // Bad Request
    if(parseCommand(data))
    {
        retCode = 200; // OK
    }
    else
    {
        retCode = 405; // Method Not Allowed
    }
    return retCode;
}

bool WbInfraRed::parseCommand(const char* data)
{
    bool commandOk = false;
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
                commandOk = true;
            }
        }
    }

    if(commandOk)
    {
        commandOk = sendToServer(wunderbar::characteristics::sensor::DATA_W,
                                 reinterpret_cast<uint8_t*>(&dataDown),
                                 sizeof(dataDown));
    }

    return commandOk;
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

size_t WbInfraRed::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuateSpecFormat[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"TX\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":255"
            "}"
        "]"
    "}";

    return snprintf(dst,
                    maxLen,
                    actuateSpecFormat,
                    config.name.c_str(),
                    config.name.c_str());
}
