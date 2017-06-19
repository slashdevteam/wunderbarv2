#pragma once

#include "Stream.h"
#include "deviceclass.h"
#include "PlatformMutex.h"

namespace usb
{

class CDC : public mbed::Stream
{
public:
    CDC();

    void run();

protected:
    virtual int _putc(int c) override;
    virtual int _getc() override;
    virtual void lock();
    virtual void unlock();

private:
    DeviceClass cdcDevice;
    PlatformMutex mutex;
};

}
