#pragma once

#include "Stream.h"
#include "CircularBuffer.h"
#include "Callback.h"
#include "deviceclass.h"

namespace usb
{

class CDC : public mbed::Stream
{
public:
    CDC();

    virtual int _putc(int c) override;
    virtual int _getc() override;

private:
    DeviceClass cdcDevice;
};

}
