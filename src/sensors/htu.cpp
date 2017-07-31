#include "htu.h"

Htu::Htu(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameHtu),
                        PassKey(defaultPass),
                        mbed::callback(this, &Htu::wunderbarEvent),
                        _proto)
{
}

void Htu::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            createJsonDataHtu(mqttClient.getPublishBuffer(), MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<sensor_htu_data_t*>(data));
            mqttClient.publish();
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
