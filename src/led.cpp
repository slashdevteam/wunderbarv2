#include "led.h"

#include "jsondecode.h"

Led::Led(Resources* _resources, const std::string& name, PinName _pin, IStdInOut& _log)
    : Resource(_resources,
               name,
               name,
               _log),
      state(0),
      ledPin(_pin),
      publisher(osPriorityNormal, 0x400, nullptr, "LED_PUBLISHER")
{
    publisher.start(mbed::callback(this, &Led::readAndPub));
}

Led::~Led()
{
    pubTick.detach();
}

int32_t Led::read()
{
    return ledPin.read();
}

void Led::pingPub()
{
    publisher.signal_set(READ_AND_PUB);
}

void Led::readAndPub()
{
    while(1)
    {
        rtos::Thread::signal_wait(READ_AND_PUB);
        publish(mbed::callback(this, &Led::toJson), nullptr);
    }
}

size_t Led::toJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const char pubFormat[] = "\"state\":%ld";
    return std::snprintf(outputString, maxLen, pubFormat, read());
}

void Led::advertise(IPubSub* _proto)
{
    Resource::advertise(_proto);
    Resource::subscribe();
    pubTick.attach(mbed::callback(this, &Led::pingPub), 30.0);
}

void Led::revoke()
{
    pubTick.detach();
    Resource::revoke();
}

void Led::handleCommand(const char* id, const char* data)
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

bool Led::parseCommand(const char* data)
{
    bool commandOk = false;
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
                commandOk = true;
            }
        }
        else if(message.isField("toggleState"))
        {
            ledPin = !ledPin;
            commandOk = true;
        }
    }

    return commandOk;
}

size_t Led::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormat[] = "{"
                "\"name\":\"LED\","
                "\"id\":\"LED\","
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

    return std::snprintf(dst,
                         maxLen,
                         senseSpecFormat);
}

size_t Led::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuaSpecFormat[] =
    "{"
        "\"name\":\"LED\","
        "\"id\":\"LED\","
        "\"commands\":"
        "["
            "{"
                "\"CommandName\":\"setState\","
                "\"DataListe\":"
                "[{"
                    "\"ValueName\":\"state\","
                    "\"ValueType\":\"integer\","
                    "\"min\":0,"
                    "\"max\":1"
                "}]"
            "},"
            "{"
                "\"CommandName\":\"toggleState\""
            "}"
        "]"
    "}";

    return std::snprintf(dst,
                         maxLen,
                         actuaSpecFormat);
}
