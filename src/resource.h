#pragma once

#include <string>
#include <memory>
#include "Callback.h"
#include "ipubsub.h"

class IPubSub;

class Resource
{
public:
    Resource(IPubSub* _proto);

    bool publish(const std::string& topic,
                 const char* data,
                 MessageDoneCallback doneCallback);

    bool subscribe(const std::string& topic,
                   MessageDoneCallback doneCallback,
                   MessageDataCallback datacallback);
                   
    bool acknowledge(const std::string& topic,
                     const std::string& _command,
                     const std::string& _code,
                     MessageDoneCallback doneCallback);

private:
    IPubSub* proto;
    std::string message;
};
