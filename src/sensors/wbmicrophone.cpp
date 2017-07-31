#include "wbmicrophone.h"

WbMicrophone::WbMicrophone(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(sensorNameMicrophone),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbMicrophone::wunderbarEvent),
                      _proto)
{
};

void WbMicrophone::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            dataToJson(mqttClient.getPublishBuffer(), MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<const sensor_microphone_data_t*>(data));
            mqttClient.publish();
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