#include "button.h"

Button::Button(IPubSub* _proto, const std::string& _topic, PinName _pin)
    : Resource(_proto,
               std::string("none"),
               _topic),
      counter(0),
      buttonIrq(_pin)
{
    buttonIrq.fall(mbed::callback(this, &Button::irqCounter));
}

void Button::irqCounter()
{
    counter++;
    publish();
}
