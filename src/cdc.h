#pragma once

#include "deviceclass.h"
#include "PlatformMutex.h"
#include "NonCopyable.h"
#include "istdinout.h"

constexpr size_t INTERNAL_BUFFER_LEN = 120;
namespace usb
{

class CDC : public IStdInOut, private mbed::NonCopyable<CDC>
{
public:
    CDC(uint8_t controllerid, device_specific_descriptors& deviceSpecificDescriptor);

    virtual int putc(int c) override;
    virtual int getc() override;
    virtual int puts(const char *str) override;
    virtual int printf(const char *format, ...) override;

private:
    void lock();
    void unlock();

private:
    DeviceClass cdcDevice;
    PlatformMutex mutex;
    char internalBuffer[INTERNAL_BUFFER_LEN];
};

}
