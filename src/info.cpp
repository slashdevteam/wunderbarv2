#include "info.h"

#include "jsondecode.h"

extern "C" void getCpuId(uint32_t* part0,
                         uint32_t* part1,
                         uint32_t* part2,
                         uint32_t* part3);

Info::Info(Resources* _resources, IStdInOut& _log)
    : Resource(_resources,
               "wbinfo",
               "wbinfo",
               _log),
      pingInterval(30)
{
    getCpuId(&serialNo[0], &serialNo[1], &serialNo[2], &serialNo[3]);
}

void Info::advertise(IPubSub* _proto)
{
    Resource::advertise(_proto);
    Resource::subscribe();
}

int Info::getPingInterval() const
{
    return pingInterval;
}

void Info::setPingChangeCallback(PingChangeCallback callback)
{
    pingCallback = callback;
}

void Info::handleCommand(const char* id, const char* data)
{
    int retCode = 400; // Bad Request
    if(parseCommand(data))
    {
        retCode = 200; // OK
    }
    else
    {
        retCode = 404; // Not found
    }

    acknowledge(id, retCode);
}

bool Info::parseCommand(const char* data)
{
    bool commandOk = false;
    JsonDecode message(data, 16);

    if(message)
    {
        char valueBuffer[3]; // enough for 2 digits and \0
        if(message.copyTo("setMqttPingInterval", valueBuffer, 3))
        {
            int value = std::atoi(valueBuffer);
            if((value >= 1) && (value <= 60))
            {
                pingInterval = value;
                if(pingCallback)
                {
                    pingCallback(pingInterval * 1000);
                }
                commandOk = true;
            }
        }
        else if(message.isField("getSerialNo"))
        {
            publish(mbed::callback(this, &Info::serialToJson), nullptr);
            commandOk = true;
        }
        else if(message.isField("getFwVersion"))
        {
            publish(mbed::callback(this, &Info::versionToJson), nullptr);
            commandOk = true;
        }
        else if(message.isField("getPingInterval"))
        {
            publish(mbed::callback(this, &Info::pingToJson), nullptr);
            commandOk = true;
        }
    }

    return commandOk;
}


size_t Info::serialToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const char pubFormat[] = "\"serialNo\":\"%08lx%8lx%8lx%8lx\"";
    return std::snprintf(outputString, maxLen, pubFormat, serialNo[0], serialNo[1], serialNo[2], serialNo[3]);
}

size_t Info::versionToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const char pubFormat[] = "\"fwVersion\":\"%s\"";
    return std::snprintf(outputString, maxLen, pubFormat, fwVersion);
}

size_t Info::pingToJson(char* outputString, size_t maxLen, const uint8_t* data)
{
    const char pubFormat[] = "\"mqttPingInterval\":%ld";
    return std::snprintf(outputString, maxLen, pubFormat, pingInterval);
}
size_t Info::getSenseSpec(char* dst, size_t maxLen)
{
    const char senseSpecFormat[] = "{"
                "\"name\":\"wbinfo\","
                "\"id\":\"wbinfo\","
                "\"data\":"
                "["
                    "{"
                        "\"name\":\"serialNo\","
                        "\"type\":\"String\""
                    "},"
                    "{"
                        "\"name\":\"fwVersion\","
                        "\"type\":\"String\""
                    "},"
                    "{"
                        "\"name\":\"mqttPingInterval\","
                        "\"type\":\"integer\","
                        "\"min\":1,"
                        "\"max\":60"
                    "}"
                "]"
           "}";

    return std::snprintf(dst,
                         maxLen,
                         senseSpecFormat);
}

size_t Info::getActuateSpec(char* dst, size_t maxLen)
{
    const char actuaSpecFormat[] =
    "{"
        "\"name\":\"wbinfo\","
        "\"id\":\"wbinfo\","
        "\"commands\":"
        "["
            "{"
                "\"CommandName\":\"setMqttPingInterval\","
                "\"DataListe\":"
                "[{"
                    "\"ValueType\":\"integer\","
                    "\"min\":1,"
                    "\"max\":60"
                "}]"
            "},"
            "{"
                "\"CommandName\":\"getPingInterval\""
            "},"
            "{"
                "\"CommandName\":\"getSerialNo\""
            "},"
            "{"
                "\"CommandName\":\"getFwVersion\""
            "}"
        "]"
    "}";

    return std::snprintf(dst,
                         maxLen,
                         actuaSpecFormat);
}
