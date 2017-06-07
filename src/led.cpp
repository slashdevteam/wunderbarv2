#include "led.h"

Led::Led(Protocol* _proto, const std::string& _topic, PinName _pin)
    : Resource(_proto, _topic),
      state(0),
      ledPin(_pin),
      subscriber(osPriorityNormal, 4096)
{
    subscriber.start(mbed::callback(this, &Led::subscribeThread));
}

bool Led::subscribe()
{
    return recv(mbed::callback(this, &Led::subscribeCallback));
}

void Led::subscribeCallback(const char* _command)
{
    lock();
    commandPtr.reset(_command);
    unlock();
    subscriber.signal_set(NEW_SUB_SIGNAL);
}

void Led::subscribeThread()
{
    while(1)
    {
        rtos::Thread::signal_wait(NEW_SUB_SIGNAL);
        std::string command = "Toggle";
        std::string code = "200";
        lock();
        sendCommandAck(command, code);
        commandPtr.reset();
        unlock();
    }
}

int32_t Led::read()
{
    return ledPin.read();
}

void Led::lock()
{
    mutex.lock();
}

void Led::unlock()
{
    mutex.unlock();
}
