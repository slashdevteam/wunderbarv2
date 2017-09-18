#include "wbbridge.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"
#include <limits>

WbBridge::WbBridge(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_BRIDGE)),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbBridge::event),
                      _resources)
{
};

void WbBridge::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            dataToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<const sensor_bridge_data_t*>(data));
            publish();
            break;
        case BleEvent::DATA_SENSOR_CONFIG:
                // not used yet
            break;
        default:
            break;
    }
}

int WbBridge::dataToJson(char* outputString, size_t maxLen, const sensor_bridge_data_t& data)
{
    const char* jsonFormatBegin = "{\"up_ch_payload\":[";
    const char* jsonFormatEnd   = "]}";
    size_t totLen = snprintf(outputString, maxLen, jsonFormatBegin);

    for (auto dataChar = 0; (dataChar < data.payload_length && totLen < maxLen); ++dataChar)
    {
        // try to write at the end of last write, for the remaining available len
        size_t lenWritten = snprintf(outputString + totLen,
                                     maxLen - totLen,
                                     "%d,",
                                     static_cast<int>(data.payload[dataChar]));

        if (0 < lenWritten)
        {
            totLen += lenWritten;
        }
        else
        {
            //error
            return totLen;
        }
    }

    // remove last coma and finish the string
    size_t lenWritten = snprintf(outputString + totLen - 1, maxLen - totLen + 1, jsonFormatEnd);
    if (0 < lenWritten)
    {
        totLen += lenWritten - 1;
    }

    return totLen;
}

size_t WbBridge::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormat[] = "{"
        "\"name\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"upstream\","
                "\"type\" : {"
                    "\"type\" : \"array\","
                    "\"maxItems\" : %ld,"
                    "\"items\": {"
                        "\"type\":\"integer\","
                        "\"min\":0,"
                        "\"max\":255"
                            "}"
                    "}"
            "},"
            "%s"
        "]"
    "}";

    return snprintf(dst,
                    maxLen,
                    senseSpecFormat,
                    config.name.c_str(),
                    BRIDGE_PAYLOAD_SIZE,
                    WunderbarSensor::getSenseSpec());
}

size_t WbBridge::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuateSpecFormat[] = "{"
        "\"name\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"downstream\","
                "\"type\" : {"
                    "\"type\" : \"array\","
                    "\"maxItems\" : %ld,"
                    "\"items\": {"
                        "\"type\":\"integer\","
                        "\"min\":0,"
                        "\"max\":255"
                            "}"
                    "}"
            "}"
        "]"
    "}";
     
    return snprintf(dst,
                    maxLen,
                    actuateSpecFormat,
                    config.name.c_str(),
                    BRIDGE_PAYLOAD_SIZE);
}
