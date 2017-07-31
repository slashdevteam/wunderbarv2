#include "gyro.h"

Gyro::Gyro(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameGyro),
                        PassKey(defaultPass),
                        mbed::callback(this, &Gyro::wunderbarEvent),
                        _proto)
{
};

void Gyro::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            createJsonDataGyro(mqttClient.getPublishBuffer(), MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<sensor_gyro_data_t*>(data));
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