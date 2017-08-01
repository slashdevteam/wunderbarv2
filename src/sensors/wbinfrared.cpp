#include "wbinfrared.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"

WbInfraRed::WbInfraRed(IBleGateway& _gateway, IPubSub* _proto)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_IR)),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbInfraRed::event),
                      _proto)
{
};

void WbInfraRed::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        default:
          break;
    }
}
