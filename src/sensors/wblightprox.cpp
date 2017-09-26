#include "wblightprox.h"
#include "wunderbarble.h"
#include "randompasskey.h"

WbLightProx::WbLightProx(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_LIGHT)),
                      randomPassKey(),
                      mbed::callback(this, &WbLightProx::event),
                      _resources)
{
};

void WbLightProx::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            dataToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<const sensor_lightprox_data_t*>(data));
            publish();
            break;
        case BleEvent::DATA_SENSOR_FREQUENCY:
            // not used yet
            break;
        case BleEvent::DATA_SENSOR_THRESHOLD:
            // not used yet
            break;
        case BleEvent::DATA_SENSOR_CONFIG:
            // not used yet
            break;
        default:
            break;
    }
}

size_t WbLightProx::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead  [] = "{"
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
