#pragma once

#include <cstdint>
#include <cstddef>

// mbed
#include "Callback.h"

class ITransportLayer;

using MessageDoneCallback = mbed::Callback<void(bool)>;
using MessageDataCallback = mbed::Callback<void(const uint8_t*, size_t)>;

class IPubSub
{
public:
    IPubSub(ITransportLayer* _transport, const char(&_name)[5])
        : name{_name[0], _name[1], _name[2], _name[3], _name[4]},
          transport(_transport)
    {}
    virtual ~IPubSub() = default;

    virtual bool connect() = 0;
    virtual bool disconnect() = 0;

    virtual bool publish(const uint8_t* dest,
                         const uint8_t* data,
                         size_t len,
                         MessageDoneCallback callback) = 0;
    virtual bool subscribe(const uint8_t* source,
                           MessageDoneCallback doneCallback,
                           MessageDataCallback callback) = 0;

public:
    const char name[5];

protected:
    ITransportLayer* transport;
};
