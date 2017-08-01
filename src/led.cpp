#include "led.h"

Led::Led(IPubSub* _proto, const std::string& _topic, PinName _pin)
    : Resource(_proto,
               _topic,
               std::string("none")),
      state(0),
      ledPin(_pin)
{
}

int32_t Led::read()
{
    return ledPin.read();
}
