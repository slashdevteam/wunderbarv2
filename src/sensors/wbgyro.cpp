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
    const char senseSpecFormat[] = "{"
        "\"name\":\"%s\","
        "\"data\":"
        "["
            "{"
                "\"name\":\"gyro_xyz\","
                "\"type\" : {"
                    "\"type\" : \"array\","
                    "\"maxItems\" : 3,"
                    "\"minItems\" : 3,"
                    "\"items”: {"
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
                    "\"items”: {"
                        "\"type\":\"integer\","
                        "\"min\":-23768,"
                        "\"max\":32767"
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
                    WunderbarSensor::getSenseSpec());
}
