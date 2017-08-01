#include "wbhtu.h"

WbHtu::WbHtu(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameHtu),
                        PassKey(defaultPass),
                        mbed::callback(this, &WbHtu::wunderbarEvent),
                        _proto)
{
}

void WbHtu::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            dataToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<const sensor_htu_data_t*>(data));
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
