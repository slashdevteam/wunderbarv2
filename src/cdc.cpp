#include "cdc.h"
#include <cstdarg>

namespace usb
{

CDC::CDC(uint8_t controllerid, device_specific_descriptors& deviceSpecificDescriptor)
    : cdcDevice(controllerid, deviceSpecificDescriptor)
{
    cdcDevice.run();
}

int CDC::putc(int c)
{
    lock();
    int ret = cdcDevice.send((uint8_t*)&c, 1);
    unlock();
    return (ret == 0) ? 1 : EOF;
}

int CDC::getc()
{
    lock();
    uint8_t c;
    int ret = cdcDevice.recv(&c, 1);
    unlock();
    return (ret != -1) ? c : EOF;
}

int CDC::puts(const char *str)
{
    lock();
    while(*str)
    {
        putc(*str++);
    }
    unlock();
    return 0;
}

int CDC::printf(const char *format, ...)
{
    lock();
    std::va_list arg;
    va_start(arg, format);
    // ARMCC microlib does not properly handle a size of 0.
    // As a workaround supply a dummy buffer with a size of 1.
    char dummy_buf[1];
    int len = vsnprintf(dummy_buf, sizeof(dummy_buf), format, arg);
    if(static_cast<size_t>(len) < sizeof(internalBuffer))
    {
        vsprintf(internalBuffer, format, arg);
        puts(internalBuffer);
    }
    else
    {
        char *temp = new char[len + 1];
        vsprintf(temp, format, arg);
        puts(temp);
        delete[] temp;
    }
    va_end(arg);
    unlock();
    return len;
}

void CDC::lock()
{
    mutex.lock();
}

void CDC::unlock()
{
    mutex.unlock();
}

}
