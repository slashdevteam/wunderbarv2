#include "button.h"

Button::Button(Protocol* _proto, const std::string& _topic, PinName _pin)
    : Resource(_proto, _topic),
      counter(0),
      buttonIrq(_pin),
      publisher(osPriorityNormal, 2*8196)
{
    buttonIrq.fall(mbed::callback(this, &Button::irqCounter));
    publisher.start(mbed::callback(this, &Button::publishThread));
}

void Button::irqCounter()
{
    counter++;
    publisher.signal_set(BUTTON_IRQ_SIGNAL);
}

void Button::publishThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(BUTTON_IRQ_SIGNAL);
        std::string data = "\"counter\":";
        data.append(std::to_string(counter));
        send(data);
    }
}
