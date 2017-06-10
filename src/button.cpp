#include "button.h"

Button::Button(IPubSub* _proto, const std::string& _topic, PinName _pin)
    : Resource(_proto),
      pubtopic(_topic),
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
        publish(pubtopic, data, mbed::callback(this, &Button::publishDone));
        rtos::Thread::signal_wait(PUBLISH_DONE_SIGNAL);
    }
}

void Button::publishDone(bool status)
{
    publisher.signal_set(PUBLISH_DONE_SIGNAL);
}
