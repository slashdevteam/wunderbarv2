#include "wbmicrophone.h"
#include "wunderbarble.h"

WbMicrophone::WbMicrophone(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_SOUND)),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbMicrophone::event),
                      _resources)
{
    const char senseSpecFormat[] = "{"
    "\"name\":\"%s\","
    "\"data\":"
    "["
        "{"
            "\"name\":\"level\","
            "\"type\":\"integer\","
            "\"min\":0,"
            "\"max\":65535"
        "},"
        "%s"
    "]"
"}";

snprintf(senseSpec,
         sizeof(senseSpec),
         senseSpecFormat,
         config.name.c_str(),
         WunderbarSensor::getSenseSpec());
};

void WbMicrophone::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            dataToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<const sensor_microphone_data_t*>(data));
            publish();
            break;
        case BleEvent::DATA_SENSOR_FREQUENCY:
            // not used yet
            break;
        case BleEvent::DATA_SENSOR_THRESHOLD:
            // not used yet
            break;
        default:
            break;
    }
}

const char* WbMicrophone::getSenseSpec()
{
    return senseSpec;
}
