#include "led.h"

#include "jsondecode.h"

Led::Led(Resources* _resources, const std::string& name, PinName _pin)
    : Resource(_resources,
               name,
               name),
      state(0),
      ledPin(_pin)
{
}

int32_t Led::read()
{
    return ledPin.read();
}

void Led::readAndPub()
{
    const char pubFormat[] = "\"state\":%ld";
    snprintf(publishContent, sizeof(publishContent), pubFormat, read());
    publish();
}

void Led::advertise(IPubSub* _proto)
{
    Resource::advertise(_proto);
    Resource::subscribe();
    pubTick.attach(mbed::callback(this, &Led::readAndPub), 10.0);
}

int Led::handleCommand(const char* data)
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

bool Led::parseCommand(const char* data)
{
    JsonDecode message(data, 16);

    if(message)
    {
        char valueBuffer[1];
        if(message.copyTo("setState", valueBuffer, 1))
        {
            int value = std::atoi(valueBuffer);
            if(value == 0 || value == 1)
            {
                ledPin = value;
                return true;
            }
            else
            {
                return false;
            }
        }
        else if(message.copyTo("toggleState", valueBuffer, 1))
        {
            int value = std::atoi(valueBuffer);
            if(value == 1)
            {
                ledPin = !ledPin;
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

const char* Led::getSenseSpec()
{
    return "{"
                "\"name\":\"LED\","
                "\"data\":"
                "["
                    "{"
                        "\"name\":\"state\","
                        "\"type\":\"integer\","
                        "\"min\":0,"
                        "\"max\":1"
                    "}"
                "]"
           "}";
}

const char* Led::getActuateSpec()
{
    return "{"
                "\"name\":\"LED\","
                "\"data\":"
                "["
                    "{"
                        "\"name\":\"setState\","
                        "\"type\":\"integer\","
                        "\"min\":0,"
                        "\"max\":1"
                    "},"
                    "{"
                        "\"name\":\"toggleState\","
                        "\"type\":\"boolean\""
                    "}"
                "]"
           "}";
}
