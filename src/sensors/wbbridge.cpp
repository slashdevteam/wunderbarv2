#include "wbbridge.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"
#include <limits>
#include "randompasskey.h"
#include "jsondecode.h"

#include "istdinout.h"
extern IStdInOut* stdioRetarget;

WbBridge::WbBridge(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_BRIDGE)),
                      randomPassKey(),
                      mbed::callback(this, &WbBridge::event),
                      _resources),
    relayState(0)
{
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

void WbBridge::handleCommand(const char* id, const char* data)
{
    bool commandOk = false;
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
            else if(message.isField("toggleState"))
            {
                relayState = !relayState;
                commandOk = true;
            }
        }

        if(commandOk)
        {
            dataDown.payload[0] = relayState;
            if(sendToServer(wunderbar::characteristics::sensor::DATA_W,
                                     reinterpret_cast<uint8_t*>(&dataDown),
                                     sizeof(dataDown)))
            {
                retCode = 200;
            }
        }
    }
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
    const char actuateSpecFormatHead[] =
    "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"commands\":"
        "["
            "{"
                "\"CommandName\":\"setState\","
                "\"DataListe\":"
                "[{"
                    "\"ValueName\":\"state\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":1"
                "}]"
            "},"
            "{"
                "\"CommandName\":\"toggleState\""
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
