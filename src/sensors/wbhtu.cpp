#include "wbhtu.h"
#include "wunderbarble.h"
#include "randompasskey.h"

WbHtu::WbHtu(IBleGateway& _gateway, Resources* _resources, IStdInOut& _log)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_HTU)),
                      randomPassKey(),
                      mbed::callback(this, &WbHtu::event),
                      _resources,
                      _log)
{
}

void WbHtu::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            publish(mbed::callback(this, &WbHtu::dataToJson), data);
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

size_t WbHtu::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"temp\","
                "\"type\":\"integer\","
                "\"min\":-50,"
                "\"max\":100"
            "},"
            "{"
                "\"name\":\"hum\","
                "\"type\":\"integer\","
                "\"min\":0,"
                "\"max\":100"
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

size_t WbHtu::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuateSpecFormatHead[] =
    "{"
        "\"name\":\"%s\","
        "\"id\":\"%s\","
        "\"commands\":"
        "[";

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
