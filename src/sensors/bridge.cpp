#include "bridge.h"

Bridge::Bridge(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameBridge),
                        PassKey(defaultPass),
                        mbed::callback(this, &Bridge::wunderbarEvent),
                        _proto)
{
};

void Bridge::wunderbarEvent(BleEvent event, uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            createJsonDataBridge(mqttClient.getPublishBuffer(), MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<sensor_bridge_data_t*>(data));
            mqttClient.publish();
        break;

        case BleEvent::DATA_SENSOR_CONFIG:
                // not used yet
        break;

        default:
        break;
    }
}