#include "wblightprox.h"

WbLightProx::WbLightProx(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(sensorNameLightProx),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbLightProx::wunderbarEvent),
                      _proto)
{
};

void WbLightProx::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    switch (event)
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