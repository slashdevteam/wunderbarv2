#include "button.h"
#include "jsmn.h"
#include "jsondecode.h"
#include "loopsutil.h"

Button::Button(Flash& _flash, Resources* _resources, const std::string& name, PinName _pin)
    : Resource(_resources,
               name,
               name),
      flash(_flash),
      publishing(false),
      counter(0),
      buttonIrq(_pin),
      buttonIrqTiming(osPriorityHigh, 0x600, nullptr, "BUTTON_TIMING")
{
    // Threads cannot be started from IRQ callbacks, so button press timing thread
    // needs to be started here
    buttonIrqTiming.start(mbed::callback(this, &Button::waitForRise));
    // button can generate spurious 'clicks' so attaching only one edge at a time
    buttonIrq.fall(mbed::callback(this, &Button::irqFall));
}

void Button::advertise(IPubSub* _proto)
{
    Resource::advertise(_proto);
    publishing = true;
}

void Button::revoke()
{
    publishing = false;
    Resource::revoke();
}

void Button::irqFall()
{
    counter++;
    // falling edge registered so switch edge detection to rising
    buttonIrq.fall(mbed::Callback<void()>());
    buttonIrq.rise(mbed::callback(this, &Button::irqRise));
    buttonIrqTiming.signal_set(BUTTON_PRESSED);

}

void Button::irqRise()
{
    // rising edge registered so switch edge detection to falling
    buttonIrq.rise(mbed::Callback<void()>());
    buttonIrq.fall(mbed::callback(this, &Button::irqFall));
    buttonIrqTiming.signal_set(BUTTON_RELEASED);
}

size_t Button::toJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    return std::snprintf(outputString, maxLen, "\"count\":%u", counter);
}


void Button::waitForRise()
{
    while(true)
    {
        // wait forever for BUTTON_PRESSED
        rtos::Thread::signal_wait(BUTTON_PRESSED);
        if(publishing)
        {
            publish(mbed::callback(this, &Button::toJson), nullptr);
        }
        // wait for BUTTON_RELEASED
        osEvent waitEvent;
        waitEvent = rtos::Thread::signal_wait(BUTTON_RELEASED, BUTTON_RESET_TIME_MS);
        // if button is pressed for more than BUTTON_RESET_TIME_MS
        // indicate to user that WB is primed for clearing onboarding
        if(osEventTimeout == waitEvent.status)
        {
            IStdInOut devNull;
            mbed::DigitalOut led(LED1);
            ProgressBar progressBar(devNull, led, true, 30);
            progressBar.start();
            // wait forever for BUTTON_RELEASED
            rtos::Thread::signal_wait(BUTTON_RELEASED);
            // clear onboarding - Wunderbar will reset!
            clearFlash();
        }
    }
}

void Button::clearFlash()
{
    flash.resetHeader();
    NVIC_SystemReset();
}

void Button::handleCommand(const char* id, const char* data)
{
    int retCode = 400; // Bad Request
    if(parseCommand(data))
    {
        retCode = 200; // OK
    }
    else
    {
        retCode = 405; // Method Not Allowed
    }
    acknowledge(id, retCode);
}

bool Button::parseCommand(const char* data)
{
    JsonDecode message(data, 16);

    if(message)
    {
        char valueBuffer[1];
        if(message.copyTo("resetCounter", valueBuffer, 1))
        {
            int value = std::atoi(valueBuffer);
            if(value == 1)
            {
                counter = 0;
                return true;
            }
            else
            {
                return false;
            }
        }
    }

    return false;
}

size_t Button::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormat[] =  "{"
                "\"name\":\"BUTTON\","
                "\"id\":\"BUTTON\","
                "\"data\":"
                "["
                    "{"
                        "\"name\":\"count\","
                        "\"type\":\"integer\","
                        "\"min\":0,"
                        "\"max\":65535"
                    "}"
                "]"
           "}";

    return snprintf(dst,
                    maxLen,
                    senseSpecFormat);
}

size_t Button::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuaSpecFormat[] =  "{"
                "\"name\":\"BUTTON\","
                "\"id\":\"BUTTON\","
                "\"data\":"
                "["
                    "{"
                        "\"name\":\"resetCounter\","
                        "\"type\":\"integer\","
                        "\"min\":1,"
                        "\"max\":1"
                    "}"
                "]"
           "}";

    return snprintf(dst,
                    maxLen,
                    actuaSpecFormat);
}
