#pragma once

#include <cstdarg>
#include <cstddef>
// catch errors from mbed to avoid printing on non-existent UART
// error in mbed-os/platform/mbed_error.c attempts to print over serial even
// if serial is explicitly not connected!
extern "C" void error(const char* format, ...);

// mbed tls also attempt to use UART serial
int cdcPrintfRetarget(const char *format, ...);
