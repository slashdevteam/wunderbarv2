#include "wbinfrared.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"

WbInfraRed::WbInfraRed(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_IR)),
                      PassKey(defaultPass),
                      mbed::callback(this, &WbInfraRed::event),
                      _resources)
{
const char actuateSpecFormat[] = "{"
    "\"name\":\"%s\","
    "\"data\":"
    "["
        "{"
            "\"name\":\"TX\","
            "\"type\":\"integer\","
            "\"min\":0,"
            "\"max\":255"
        "},"
        "%s"
    "]"
"}";
     
snprintf(actuateSpec,
        sizeof(actuateSpec),
        actuateSpecFormat,
        config.name.c_str(),
        WunderbarSensor::getActuateSpec());
};

void WbInfraRed::event(BleEvent _event, const uint8_t* data, size_t len)
{
    switch(_event)
    {
        default:
          break;
    }
}

const char* WbInfraRed::getActuateSpec()
{
    return actuateSpec;
}
