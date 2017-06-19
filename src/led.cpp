#include "led.h"

Led::Led(IPubSub* _proto, const std::string& _topic, PinName _pin)
    : Resource(_proto),
      subtopic(_topic),
      acktopic(_topic + "ack"),
      state(0),
      ledPin(_pin),
      subscriber(osPriorityNormal, 1024),
      subscribed(false)
{
    subscriber.start(mbed::callback(this, &Led::subscribeThread));
}

bool Led::subscribe()
{
    return Resource::subscribe(subtopic,
                               mbed::callback(this, &Led::subscribeDone),
                               mbed::callback(this, &Led::subscribeCallback));
}

void Led::subscribeDone(bool status)
{
    subscribed = status;
}

void Led::subscribeCallback(const uint8_t* data, size_t len)
{
    command = std::make_unique<uint8_t[]>(len);
    std::memcpy(command.get(), data, len);
    subscriber.signal_set(NEW_SUB_SIGNAL);
}

void Led::subscribeThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_SUB_SIGNAL);
        if(subscribed)
        {
            std::string commandName = "Toggle";
            std::string code = "200";
            acknowledge(acktopic, commandName, code, mbed::callback(this, &Led::ackDone));
            rtos::Thread::signal_wait(ACK_DONE_SIGNAL);
            ledPin = !ledPin;
            command.reset();
        }
    }
}

int32_t Led::read()
{
    return ledPin.read();
}

void Led::ackDone(bool status)
{
    subscriber.signal_set(ACK_DONE_SIGNAL);
}
