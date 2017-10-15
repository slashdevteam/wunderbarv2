#include "wbbridge.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"
#include <limits>
#include "randompasskey.h"
#include "jsondecode.h"

#include "istdinout.h"
extern IStdInOut* stdioRetarget;

WbBridge::WbBridge(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_BRIDGE)),
                      randomPassKey(),
                      mbed::callback(this, &WbBridge::event),
                      _resources,
                      _log),
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
            publish(mbed::callback(this, &WbBridge::configToJson), data);
            break;
        default:
            break;
    }
}

void WbBridge::handleCommand(const char* id, const char* data)
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
            char valueBuffer[1];
            if(message.copyTo("setState", valueBuffer, 1))
            {
                int value = std::atoi(valueBuffer);
                if(value == 0 || value == 1)
                {
                    relayState = value;
                    dataDown.payload[0] = relayState;
                    if(sendToServer(wunderbar::characteristics::sensor::DATA_W,
                                             reinterpret_cast<uint8_t*>(&dataDown),
                                             sizeof(dataDown)))
                    {
                        retCode = 200;
                    }
                }
            }
            else if(message.isField("toggleState"))
            {
                relayState = !relayState;
                dataDown.payload[0] = relayState;
                if(sendToServer(wunderbar::characteristics::sensor::DATA_W,
                                         reinterpret_cast<uint8_t*>(&dataDown),
                                         sizeof(dataDown)))
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
                char baudRateBuffer[11];
                if(message.copyTo("baudRate", baudRateBuffer, 1))
                {
                    int value = std::atoi(baudRateBuffer);
                    if(isBaudrateAllowed(value))
                    {
                        if(sendToServer(wunderbar::characteristics::sensor::CONFIG,
                                        reinterpret_cast<uint8_t*>(&value),
                                        sizeof(value)))
                        {
                            retCode = 200;
                        }
                    }
                }
            }
        }
    }
}

size_t WbBridge::configToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString,
                         maxLen,
                         "\"baudrate\":%d",
                         static_cast<int>(data[0]));
}

bool WbBridge::isBaudrateAllowed(int baudRate)
{
    bool allowed = true;

    switch(baudRate)
    {
        case 1200:   // intentional fall-through
        case 2400:   // intentional fall-through
        case 4800:   // intentional fall-through
        case 9600:   // intentional fall-through
        case 14400:  // intentional fall-through
        case 19200:  // intentional fall-through
        case 28800:  // intentional fall-through
        case 38400:  // intentional fall-through
        case 57600:  // intentional fall-through
        case 76800:  // intentional fall-through
        case 115200: // intentional fall-through
        case 230400: // intentional fall-through
        case 250000: // intentional fall-through
        case 460800: // intentional fall-through
        case 921600:
            allowed = true;
            break;
        default:
            allowed = false;
            break;
    }
    return allowed;
}

size_t WbBridge::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"baudrate\","
                "\"type\":\"integer\","
                "\"min\":1200,"
                "\"max\":921600"
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
                "\"CommandName\":\"getConfig\""
            "},"
            "{"
                "\"CommandName\":\"setConfig\""
                "\"DataListe\":"
                "[{"
                    "\"ValueName\":\"baudRate\","
                    "\"ValueType\":\"integer\","
                    "\"min\":1200,"
                    "\"max\":921600"
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
