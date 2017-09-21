#include "wbinfrared.h"
#include "wunderbarsensordatatypes.h"
#include "wunderbarble.h"
#include "randompasskey.h"

WbInfraRed::WbInfraRed(IBleGateway& _gateway, Resources* _resources)
    : WunderbarSensor(_gateway,
                      ServerName(WunderbarSensorNames(wunderbar::sensors::DATA_ID_DEV_IR)),
                      randomPassKey(),
                      mbed::callback(this, &WbInfraRed::event),
                      _resources)
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

size_t WbInfraRed::getActuateSpec(char* dst, size_t maxLen)
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
            "}"
        "]"
    "}";

    return snprintf(dst,
                    maxLen,
                    actuateSpecFormat,
                    config.name.c_str());
}
