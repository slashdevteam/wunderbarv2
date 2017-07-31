#include "lightprox.h"

LightProx::LightProx(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(sensorNameLightProx),
                      PassKey(defaultPass),
                      mbed::callback(this, &LightProx::wunderbarEvent),
                      _proto)
{
};

void LightProx::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            createJsonDataLight(mqttClient.getPublishBuffer(), MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<sensor_lightprox_data_t*>(data));
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