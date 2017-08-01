#include "wbhtu.h"
#include "wunderbarble.h"

WbHtu::WbHtu(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_HTU)),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbHtu::event),
                      _proto)
{
}

void WbHtu::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
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
