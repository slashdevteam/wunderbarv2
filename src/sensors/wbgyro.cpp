#include "wbgyro.h"
#include "wunderbarble.h"

WbGyro::WbGyro(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_GYRO)),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbGyro::event),
                      _resources)
{
};

void WbGyro::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            dataToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<const sensor_gyro_data_t*>(data));
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

size_t WbGyro::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormatHead[] = "{"
        "\"name\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"gyro_xyz\","
                "\"type\" : {"
                    "\"type\" : \"array\","
                    "\"maxItems\" : 3,"
                    "\"minItems\" : 3,"
                    "\"items\" : {"
                        "\"type\":\"integer\","
                        "\"min\":-23768,"
                        "\"max\":32767"
                            "}"
                    "}"
            "},"
            "{"
                "\"name\":\"acc_xyz\","
                "\"type\" : {"
                    "\"type\" : \"array\","
                    "\"maxItems\" : 3,"
                    "\"minItems\" : 3,"
                    "\"items\" : {"
                        "\"type\":\"integer\","
                        "\"min\":-23768,"
                        "\"max\":32767"
                            "}"
                    "}"
            "},";

    const char senseSpecFormatTail[] = 
        "]"
    "}";

    size_t sizeWritten = snprintf(dst,
                                  maxLen,
                                  senseSpecFormatHead,
                                  config.name.c_str());

    sizeWritten += WunderbarSensor::getSenseSpec(dst + sizeWritten, maxLen - sizeWritten);

    sizeWritten += snprintf(dst + sizeWritten,
                            maxLen - sizeWritten,
                            senseSpecFormatTail);

    return sizeWritten;
}
