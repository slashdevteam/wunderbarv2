#include "wbbridge.h"

WbBridge::WbBridge(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                        ServerName(sensorNameBridge),
                        PassKey(defaultPass),
                        mbed::callback(this, &WbBridge::wunderbarEvent),
                        _proto)
{
};

void WbBridge::wunderbarEvent(BleEvent event, const uint8_t* data, size_t len)
{
    switch (event)
    {
        case BleEvent::DATA_SENSOR_NEW_DATA:
            dataToJson(publishContent, MQTT_MSG_PAYLOAD_SIZE, *reinterpret_cast<const sensor_bridge_data_t*>(data));
            publish();
        break;

        case BleEvent::DATA_SENSOR_CONFIG:
                // not used yet
        break;

        default:
        break;
    }
}