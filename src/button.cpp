#include "button.h"
#include "jsmn.h"
#include "jsondecode.h"
#include "flash.h"
#include "loopsutil.h"

extern Flash flash;

const int32_t BUTTON_PRESSED = 0x1;
const int32_t BUTTON_RELEASED = 0x2;
const int BUTTON_RESET_TIME_MS = 5000;

Button::Button(Resources* _resources, const std::string& name, PinName _pin)
    : Resource(_resources,
               name,
               name),
      counter(0),
      buttonIrq(_pin),
      buttonIrqTiming(osPriorityNormal, 0x300)
{
    // Threads cannot be started from IRQ callbacks, so button press timing thread
    // needs to be started here
    buttonIrqTiming.start(mbed::callback(this, &Button::waitForRise));
    buttonIrq.fall(mbed::callback(this, &Button::irqFall));
    buttonIrq.rise(mbed::callback(this, &Button::irqRise));
}

void Button::advertise(IPubSub* _proto)
{
    Resource::advertise(_proto);
}

void Button::irqFall()
{
    buttonIrqTiming.signal_set(BUTTON_PRESSED);
    irqCounter();
}

void Button::irqRise()
{
    buttonIrqTiming.signal_set(BUTTON_RELEASED);
}

void Button::irqCounter()
{
    counter++;
    publish(mbed::callback(this, &Button::toJson), nullptr);
}

size_t Button::toJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const char pubFormat[] = "\"count\":%u";
    return std::snprintf(outputString, maxLen, pubFormat, counter);
}

void Button::waitForRise()
{
    while(true)
    {
        // wait forever for BUTTON_PRESSED
        rtos::Thread::signal_wait(BUTTON_PRESSED);

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
