#pragma once

#include "resource.h"
#include "istdinout.h"
#include "Callback.h"

using PingChangeCallback = mbed::Callback<void(int)>;

class Info : public Resource
{
public:
    Info(Resources* _resources, IStdInOut& _log);
    virtual ~Info() = default;

    virtual void advertise(IPubSub* _proto) override;
    virtual size_t getSenseSpec(char* dst, size_t maxLen) override;
    virtual size_t getActuateSpec(char* dst, size_t maxLen) override;
    int getPingInterval() const;
    void setPingChangeCallback(PingChangeCallback callback);

protected:
    virtual void handleCommand(const char* id, const char* data) override;
    bool parseCommand(const char* data);

private:
    size_t serialToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t versionToJson(char* outputString, size_t maxLen, const uint8_t* data);
    size_t pingToJson(char* outputString, size_t maxLen, const uint8_t* data);

private:
    uint32_t pingInterval;
    uint32_t serialNo[4];
    const char* fwVersion = "V2.RC2." __DATE__;
    PingChangeCallback pingCallback;
};
