#include "nouartfix.h"

#include "cdc.h"
using usb::CDC;
extern CDC cdc;

int cdcVPrintf(const char *format, va_list args)
{
    char internalBuffer[256];
    // ARMCC microlib does not properly handle a size of 0.
    // As a workaround supply a dummy buffer with a size of 1.
    char dummy_buf[1];
    int len = vsnprintf(dummy_buf, sizeof(dummy_buf), format, args);
    if(static_cast<size_t>(len) < sizeof(internalBuffer))
    {
        vsprintf(internalBuffer, format, args);
        cdc.puts(internalBuffer);
    }
    else
    {
        char *temp = new char[len + 1];
        vsprintf(temp, format, args);
        cdc.puts(temp);
        delete[] temp;
    }
    return len;
}

// avoid prints to non-existent UART forced by mbed
static uint8_t error_in_progress = 0;
extern "C" void error(const char* format, ...)
{
    // Prevent recursion if error is called again
    if (error_in_progress)
    {
        return;
    }
    std::va_list arg;
    va_start(arg, format);
    cdcVPrintf(format, arg);
    va_end(arg);
    exit(1);
}

int cdcPrintfRetarget(const char *format, ...)
{
    std::va_list arg;
    va_start(arg, format);
    int len = cdcVPrintf(format, arg);
    va_end(arg);
    return len;
}
