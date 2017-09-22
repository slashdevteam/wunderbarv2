#include "button.h"
#include "jsmn.h"
#include "jsondecode.h"
#include "flash.h"

extern Flash flash;

Button::Button(Resources* _resources, const std::string& name, PinName _pin)
    : Resource(_resources,
               name,
               name),
      counter(0),
      buttonIrq(_pin)
{
    buttonIrq.fall(mbed::callback(this, &Button::irqCounter));
}

void Button::irqCounter()
{
    counter++;
    const char pubFormat[] = "\"count\":%u";
    snprintf(publishContent, sizeof(publishContent), pubFormat, counter);
    publish();
}

int Button::handleCommand(const char* data)
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
    return retCode;
}

bool Button::parseCommand(const char* data)
{
    JsonDecode message(data, 16);

    if(message)
    {
        char valueBuffer[1];
        if(message.copyTo("resetOnboarding", valueBuffer, 1))
        {
            int value = std::atoi(valueBuffer);
            if(value == 1)
            {
                flash.resetHeader();
                NVIC_SystemReset();
                return true;
            }
            else
            {
                return false;
            }
        }
        else if(message.copyTo("resetCounter", valueBuffer, 1))
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
                    "},"
                    "{"
                        "\"name\":\"resetOnboarding\","
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
