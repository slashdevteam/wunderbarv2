#include "wbbridge.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"
#include <limits>
#include "randompasskey.h"
#include "jsondecode.h"

WbBridge::WbBridge(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_BRIDGE)),
                      randomPassKey(),
                      mbed::callback(this, &WbBridge::event),
                      _resources),
    relayState(0)
{
};

void WbBridge::advertise(IPubSub* _proto)
{
    Resource::subscribe();
}

void WbBridge::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            // bridge as relay has no data
            break;
        case BleEvent::DATA_SENSOR_CONFIG:
            // not used yet
            break;
        default:
            break;
    }
}

int WbBridge::handleCommand(const char* data)
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

bool WbBridge::parseCommand(const char* data)
{
    bool commandOk = false;
    JsonDecode message(data, 16);

    if(message)
    {
        char valueBuffer[1];
        if(message.copyTo("setState", valueBuffer, 1))
        {
            int value = std::atoi(valueBuffer);
            if(value == 0 || value == 1)
            {
                relayState = value;
                commandOk = true;
            }
        }
        else if(message.copyTo("toggleState", valueBuffer, 1))
        {
            int value = std::atoi(valueBuffer);
            if(value == 1)
            {
                relayState = !relayState;
                commandOk = true;
            }
        }
    }

    if(commandOk)
    {
        dataDown.payload[0] = relayState;
        commandOk = sendToServer(wunderbar::characteristics::sensor::DATA_W,
                                 reinterpret_cast<uint8_t*>(&dataDown),
                                 sizeof(dataDown));
    }

    return commandOk;
}

size_t WbBridge::getSenseSpec(char* dst, size_t maxLen)
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

size_t WbBridge::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuateSpecFormat[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"setState\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":1"
            "},"
            "{"
                "\"name\":\"toggleState\","
                "\"type\":\"boolean\""
            "}"
        "]"
    "}";

    return snprintf(dst,
                    maxLen,
                    actuateSpecFormat,
                    config.name.c_str(),
                    config.name.c_str());
}
