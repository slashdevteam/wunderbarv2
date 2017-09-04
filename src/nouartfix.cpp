#include "nouartfix.h"

#include "cdc.h"
using usb::CDC;
extern CDC cdc;

// avoid prints to non-existent UART forced by mbed
static uint8_t error_in_progress = 0;
extern "C" void error(const char* format, ...)
{
    // Prevent recursion if error is called again
    if (error_in_progress) {
        return;
    }
    error_in_progress = 1;
    exit(1);
}

int cdcPrintfRetarget(const char *format, ...)
{
    char internalBuffer[256];
    std::va_list arg;
    va_start(arg, format);
    // ARMCC microlib does not properly handle a size of 0.
    // As a workaround supply a dummy buffer with a size of 1.
    char dummy_buf[1];
    int len = vsnprintf(dummy_buf, sizeof(dummy_buf), format, arg);
    if(static_cast<size_t>(len) < sizeof(internalBuffer))
    {
        vsprintf(internalBuffer, format, arg);
        cdc.puts(internalBuffer);
    }
    else
    {
        char *temp = new char[len + 1];
        vsprintf(temp, format, arg);
        cdc.puts(temp);
        delete[] temp;
    }
    va_end(arg);
    return len;
}
